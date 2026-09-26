# Screen text inventory — PROPOSED

**For review. Nothing here has been applied to the source.** Confirm or
amend, and I will push the agreed wording into the app and regenerate
`screen-text-inventory.md` from it.

Counts are **lines in the 440px help column at scale 3**, measured the same
way as the current inventory. The pane allows 11.

---

## The rules applied

* **Lead with the effect.** "Randomizes enemy placements."
* **Cut implementation detail.** "drawn from the enemy pool" is
  documentation, not UI.
* **Drop `Default: Off.` everywhere.** The value column two inches to the
  left already says `YES` or `NO`. That sentence cost a line on all sixteen
  toggles and told the player nothing they could not see.
* **Drop defensive framing.** "Not a randomizer", "Different from…",
  "Independent of…" — all explaining what a setting *is not*.
* **Keep real dependencies.** "Requires Randomize Treasure" changes what a
  player does; a paragraph on why does not.
* **Game vocabulary.** Hunter's Dream, Bath Messengers, Workshop Tools,
  Shadows of Yharnam.
* **Case convention is unchanged.** Help bodies are sentence case; rail
  labels, pane values, notes and footers stay uppercase — they are chrome
  and data, and they sit beside `YES`, `ACTIVE`, `14.5 MB IN 17 FILES`.

---

## Where I disagree with the review — `ENABLE MERGO DARKNESS`

The review flagged my wording as possibly reversed, and suggested
*"Removes the Wet Nurse's darkness from Mergo's Loft."* **I checked the
code, and that suggestion would be wrong in both direction and scope.** It
reads the shipping help text as ground truth, and the shipping help text is
the thing that is wrong.

`app/src/Randomizer/PermaDarkness.h` states it in capitals, and says it was
measured on console in both directions:

```
permaDarknessOn == true   -> writes the game's own shipped values
                             -> a permanently DARK world
permaDarknessOn == false  -> writes the poke (SpEffect 99999)
                             -> the normal, LIT game
```

The same header adds **"not an m26-local effect"** — the darkness is not
confined to Mergo's Loft; it runs from the first spawn. Backlog row 8 and
`docs/user-guide.md` agree with the code and with each other.

So the shipping string is wrong twice: it has the direction backwards *and*
localises a global effect to one area. This is exactly the trap the file
warns about in its own comment — **"CAREFUL WITH THE POLARITY, AND DO NOT
REASON IT OUT FROM THE BYTES"** — because the decode of which byte means
what genuinely does suggest the opposite of what the console does.

My revised wording keeps the direction and fixes the scope, and borrows the
user guide's framing, which is the clearest of the three:

> Turns the Wet Nurse's arena darkness on for the whole game, from your
> first spawn.

**No code, setting name or value has been touched.** If you would rather
confirm on console before changing this one string, it is the one item here
worth holding back — but leaving the current text means shipping a sentence
that tells the player the opposite of what will happen.

---

## The three other questions the review raised

**`TITLE ID` — is "four letters and five digits" needed?** No, and it is
dropped. `SetupDefaultsScreen.cpp` hard-constrains the editor: positions
0-3 cycle letters, 4-8 cycle digits, over a fixed length. A player cannot
enter an invalid shape, so describing the shape is describing a rule the UI
already enforces. **"PS4 title ID" is restored** — that was a real loss.

**`HISTORY` — is "nothing is ever lost" literally true?** Not quite, so it
is gone. Revisions are append-only within a world, but deleting a world
removes its folder and every revision in it (`WorldStore.cpp:898`). The
replacement — *"so the list only grows"* — is exactly true and makes the
same point.

**Activation refusals — is the duplicated "run Bloodborne once" message
intentional?** Yes, and deliberately so. The two cases have different
causes (no save title found at all, versus a world with a stored save and
no container) but the player's action is identical, and the second can only
arise when the first is also true. One sentence for one remedy is better
than two phrasings of it, and it is the same sentence as the pane note in
§4, so the player meets one message in all three places.

**`kOutgoingNowhere` / `kOutgoingFiled` vocabulary.** Caught correctly — my
first pass still said "filed" in one and "saved" in the other. Both now
use one pair: a save is **backed up** (to `SaveBackups`) and **saved to** a
world. "Filed" is gone everywhere.

**Grammar convention, kept as the review described it.** A toggle says what
happens — *"Randomizes…"*. A picker says what the player is choosing —
*"Choose which…"*. That split is intentional and is now the rule.

---

# 1. Settings

## ENEMIES

### RANDOMIZE ENEMIES

| | Lines | Words |
| --- | --- | --- |
| Now | 5 | 21 |
| **Proposed** | **3** | **9** |

*Now:* Replaces each enemy placement in the world with another creature drawn from the enemy pool. Does not affect bosses. Default: Off.

**Proposed:** Randomizes enemy placements throughout the world. Bosses are unaffected.

### ENEMIES INCLUDED

| | Lines | Words |
| --- | --- | --- |
| Now | 5 | 21 |
| **Proposed** | **2** | **7** |

*Now:* Chooses which creatures may be used as replacements. Everything is included by default. Has no effect unless Randomize Enemies is on.

**Proposed:** Choose which enemies can appear as replacements.

### ENEMIES SKIPPED

| | Lines | Words |
| --- | --- | --- |
| Now | 7 | 34 |
| **Proposed** | **4** | **14** |

*Now:* Chooses which enemies are left exactly as the game placed them. Nothing is skipped by default. A skipped enemy can still appear elsewhere as a replacement. Has no effect unless Randomize Enemies is on.

**Proposed:** Choose enemies that keep their original placements. They may still appear as replacements elsewhere.

### DO NOT RANDOMIZE CAGED DOGS

| | Lines | Words |
| --- | --- | --- |
| Now | 9 | 44 |
| **Proposed** | **5** | **22** |

*Now:* Leaves the caged dogs of Central Yharnam and the Forbidden Woods alone. This protects the ten cage placements, not the creature - Shaggy Hunting Dogs still appear elsewhere and still feed the pool. Replacements dropped into the Central Yharnam cages misbehave badly. Default: Off.

**Proposed:** Keeps the caged dogs of Central Yharnam and the Forbidden Woods in their original placements. Other enemies behave badly in these cages.

## BOSSES

### RANDOMIZE BOSSES

| | Lines | Words |
| --- | --- | --- |
| Now | 4 | 20 |
| **Proposed** | **2** | **6** |

*Now:* Replaces each boss with another boss. Independent of Randomize Enemies - either, both or neither may be on. Default: Off.

**Proposed:** Randomizes boss placements throughout the world.

### BOSSES INCLUDED

| | Lines | Words |
| --- | --- | --- |
| Now | 5 | 21 |
| **Proposed** | **2** | **7** |

*Now:* Chooses which bosses may be used as replacements. Everything is included by default. Has no effect unless Randomize Bosses is on.

**Proposed:** Choose which bosses can appear as replacements.

## ITEMS & TREASURE

### RANDOMIZE TREASURE

| | Lines | Words |
| --- | --- | --- |
| Now | 2 | 10 |
| **Proposed** | **2** | **6** |

*Now:* Shuffles the items found lying in the world. Default: Off.

**Proposed:** Randomizes items found throughout the world.

### RANDOMIZE WORKSHOP TOOLS

| | Lines | Words |
| --- | --- | --- |
| Now | 10 | 47 |
| **Proposed** | **4** | **15** |

*Now:* Adds the two workshop tools - the Blood Gem and Rune workshop tools - to the treasure shuffle instead of leaving them where the game put them. Only meaningful when Randomize Treasure is also on. Different from Start With Hunter Tools, which grants them outright. Default: Off.

**Proposed:** Adds the Blood Gem and Rune Workshop Tools to the treasure pool. Requires Randomize Treasure.

### RANDOMIZE ENEMY DROPS

| | Lines | Words |
| --- | --- | --- |
| Now | 2 | 8 |
| **Proposed** | **2** | **6** |

*Now:* Shuffles what enemies drop when killed. Default: Off.

**Proposed:** Randomizes what enemies drop when killed.

## WEAPONS & STARTING GEAR

### RANDOMIZE STARTING WEAPONS

| | Lines | Words |
| --- | --- | --- |
| Now | 3 | 12 |
| **Proposed** | **2** | **9** |

*Now:* Randomizes the trick weapon choices offered in the Hunter's Dream. Default: Off.

**Proposed:** Randomizes the trick weapons offered in the Hunter's Dream.

### RANDOMIZE STARTING GUNS

| | Lines | Words |
| --- | --- | --- |
| Now | 4 | 17 |
| **Proposed** | **2** | **8** |

*Now:* Randomizes the firearm choices offered in the Hunter's Dream. Independent of the trick weapon setting. Default: Off.

**Proposed:** Randomizes the firearms offered in the Hunter's Dream.

### RANDOMIZE SHOP WEAPONS

| | Lines | Words |
| --- | --- | --- |
| Now | 3 | 10 |
| **Proposed** | **2** | **8** |

*Now:* Randomizes the weapons sold by the Bath Messengers. Default: Off.

**Proposed:** Randomizes the weapons sold by the Bath Messengers.

### START WITH HUNTER TOOLS

| | Lines | Words |
| --- | --- | --- |
| Now | 9 | 43 |
| **Proposed** | **4** | **19** |

*Now:* Grants the Blood Gem and Rune workshop tools at character creation, so gems and runes work from the first area instead of sitting unusable until their chests turn up. Different from Randomize Workshop Tools, which shuffles them into the treasure pool. Default: Off.

**Proposed:** Start with the Blood Gem and Rune Workshop Tools. Gems and runes can be fitted from the first area.

## DIFFICULTY

### EASY SHADOWS

| | Lines | Words |
| --- | --- | --- |
| Now | 6 | 34 |
| **Proposed** | **3** | **15** |

*Now:* Replaces the duplicate bodies in the Shadows of Yharnam fight with harmless larvae, so it plays as a duel. Not a randomizer - the same seed gives the same world either way. Default: Off.

**Proposed:** Replaces the duplicate Shadows of Yharnam with harmless larvae, so the fight is a duel.

### EASY ROM

| | Lines | Words |
| --- | --- | --- |
| Now | 2 | 8 |
| **Proposed** | **2** | **7** |

*Now:* The same, for Rom's attendant spiders. Default: Off.

**Proposed:** Replaces Rom's attendant spiders with harmless larvae.

### EASY FAILURES

| | Lines | Words |
| --- | --- | --- |
| Now | 2 | 8 |
| **Proposed** | **2** | **8** |

*Now:* The same, for the Living Failures. Default: Off.

**Proposed:** Replaces the duplicate Living Failures with harmless larvae.

### EASY EMISSARY

| | Lines | Words |
| --- | --- | --- |
| Now | 3 | 10 |
| **Proposed** | **3** | **9** |

*Now:* The same, for the Celestial Emissary's lesser emissaries. Default: Off.

**Proposed:** Replaces the Celestial Emissary's lesser emissaries with harmless larvae.

## WORLD

### ENABLE MERGO DARKNESS

| | Lines | Words |
| --- | --- | --- |
| Now | 7 | 39 |
| **Proposed** | **3** | **15** |

*Now:* Cuts the scripted darkness in Mergo's Loft. Not a randomizer - it is a single fixed edit, and it applies whether or not anything else is on. Off leaves the area exactly as the game shipped it. Default: Off.

**Proposed:** Turns the Wet Nurse's arena darkness on for the whole game, from your first spawn.

## SAVE

### SAVE DATA

| | Lines | Words |
| --- | --- | --- |
| Now | 10 | 56 |
| **Proposed** | **6** | **34** |

*Now:* What happens to save data when this world is activated. Keep Existing restores this world's own save, or adopts the live save if it has none. Start Fresh empties the container so the game starts a new playthrough. Either way the live save is backed up first, and Start Fresh lasts one activation. Default: Keep Existing.

**Proposed:** Keep Existing restores this world's save, or uses your current save if it has none. Start Fresh begins a new playthrough, and applies to this activation only. Your save is always backed up first.

---

# 2. Rail rows that are not settings

### TITLE ID — DEFAULTS tab

| | Lines | Words |
| --- | --- | --- |
| Now | 8 | 41 |
| **Proposed** | **5** | **22** |

*Now:* The PS4 title ID of the Bloodborne installation the randomizer writes for. Four letters and five digits, the shape the console itself uses. X edits it one character at a time. Default: CUSA03173, the Europe and Game of the Year release.

**Proposed:** The PS4 title ID of the Bloodborne installation the randomizer writes to. CUSA03173 is the Europe and Game of the Year release.

### NAME — editor

| | Lines | Words |
| --- | --- | --- |
| Now | 7 | 49 |
| **Proposed** | **3** | **16** |

*Now:* What this world is called. Up to sixteen characters of A to Z, 0 to 9 and space, edited one character at a time. The name is yours to change at any time - it is not part of the recipe, so renaming a world records no new revision.

**Proposed:** What this world is called. Up to 16 characters. Renaming does not create a new revision.

### SEED — editor

| | Lines | Words |
| --- | --- | --- |
| Now | 5 | 23 |
| **Proposed** | **4** | **14** |

*Now:* The number that determines this run's randomization. The same seed with the same settings always produces the same world. Left/Right rolls a new 

**Proposed:** Determines this world's randomization. The same seed and settings always produce the same world.

### HISTORY — editor

| | Lines | Words |
| --- | --- | --- |
| Now | 6 | 31 |
| **Proposed** | **4** | **22** |

*Now:* Every recipe this world has ever had, newest first. Nothing here is ever overwritten: choosing an older revision makes it current by adding a new one, so the list only grows.

**Proposed:** Every set of settings this world has had, newest first. Choosing an older one makes it current, so the list only grows.

---

# 3. WORLDS screen — help column

### + NEW WORLD

| | Lines | Words |
| --- | --- | --- |
| Now | 5 | 23 |
| **Proposed** | **2** | **8** |

*Now:* Creates a world and opens it in the editor, filled in from the DEFAULTS tab. Nothing is written until the world is activated.

**Proposed:** Create a new world using your default settings.

### VANILLA

| | Lines | Words |
| --- | --- | --- |
| Now | 5 | 27 |
| **Proposed** | **2** | **10** |

*Now:* The game as it shipped. Vanilla has no settings and cannot be deleted. Activating it removes the randomizer's files and makes Vanilla's own save data live again.

**Proposed:** Play Bloodborne with the original game files and save data.

### WORLD

| | Lines | Words |
| --- | --- | --- |
| Now | 5 | 28 |
| **Proposed** | **2** | **8** |

*Now:* One playthrough: a set of randomizer settings and the save data produced by playing them. X opens it in the editor, where it can be changed and activated.

**Proposed:** A saved randomizer setup and its save data.

---

# 4. WORLDS screen — pane notes

| String | Now | Proposed |
| --- | --- | --- |
| `kNoteNewWorld` | A NEW WORLD STARTS FROM THE DEFAULTS TAB AND IS SAVED WHEN THE EDITOR IS FINISHED | STARTS FROM YOUR DEFAULT SETTINGS **←** |
| `kNoteVanilla` | VANILLA HAS NO SETTINGS AND NO REVISIONS AND CANNOT BE DELETED | NO SETTINGS - CANNOT BE DELETED **←** |
| `kNoteNoContainer` | BLOODBORNE HAS NOT MADE SAVE DATA ON THIS CONSOLE YET - RUN THE GAME ONCE BEFORE ACTIVATING THIS WORLD | RUN BLOODBORNE ONCE BEFORE ACTIVATING THIS WORLD **←** |
| `kNoteContainerTooSmall` | THIS WORLD'S SAVE NEEDS A LARGER SAVE CONTAINER THAN THIS CONSOLE HAS | THIS WORLD'S SAVE IS TOO LARGE FOR THIS CONSOLE **←** |

---

# 5. Delete confirmation

| String | Now | Proposed |
| --- | --- | --- |
| `kDeleteTitle` | DELETE WORLD | DELETE WORLD |
| `kDeleteLine1` | THIS REMOVES THE WORLD AND EVERY REVISION IT HAS | REMOVES THIS WORLD AND ALL ITS REVISIONS **←** |
| `kDeleteLine2` | ITS SAVE DATA IS KEPT AS A SAFETY BACKUP AND IS NOT DELETED | ITS SAVE DATA IS KEPT **←** |
| `kDeleteKeptLine` | ITS SAVE DATA WAS KEPT IN SAVEBACKUPS | SAVE DATA KEPT IN SAVEBACKUPS **←** |
| `kDeleteFooter` | X DELETE   O CANCEL | X DELETE   O CANCEL |
| `kDeleteRefusedFooter` | O BACK | O BACK |
| `kRefuseVanilla` | VANILLA IS ALWAYS PRESENT AND CANNOT BE DELETED | VANILLA CANNOT BE DELETED **←** |
| `kRefuseActive` | THIS WORLD IS ACTIVE - ACTIVATE ANOTHER WORLD BEFORE DELETING IT | ACTIVE WORLDS CANNOT BE DELETED **←** |

---

# 6. Startup screen

| String | Now | Proposed |
| --- | --- | --- |
| `kWordmark` | BLOODBORNE RANDOMIZER | BLOODBORNE RANDOMIZER |
| `kLoadingWord` | LOADING | LOADING |
| `kProblemTitle` | STARTUP PROBLEM | STARTUP PROBLEM |
| `kProblemReconcileFailed` | AN INTERRUPTED ACTIVATION COULD NOT BE FINISHED | AN INTERRUPTED ACTIVATION COULD NOT BE FINISHED |
| `kProblemJournalNotUnderstood` | AN INTERRUPTED ACTIVATION WAS NOT UNDERSTOOD AND WAS LEFT ALONE | AN INTERRUPTED ACTIVATION COULD NOT BE READ - IT WAS LEFT ALONE **←** |
| `kProblemCaptureFailed` | THE EXISTING SAVE COULD NOT BE FILED INTO VANILLA | YOUR SAVE COULD NOT BE COPIED INTO VANILLA **←** |
| `kProblemNoSignedInPlayer` | NO SIGNED-IN PLAYER | NO PLAYER SIGNED IN **←** |
| `kProblemScrollHint` | UP DOWN SCROLL | UP DOWN SCROLL |
| `kPromptContinue` | X CONTINUE | X CONTINUE |
| `kPromptExit` | O EXIT | O EXIT |

---

# 7. Activation confirmation — world editor

*Outside the original inventory. Included because consistent voice is the
point, and these are the sentences a player reads immediately before the
one irreversible thing the app does.*

| String | Now | Proposed |
| --- | --- | --- |
| `kConfirmHeading` | CONFIRM ACTIVATION | CONFIRM ACTIVATION |
| `kCannotActivate` | CANNOT ACTIVATE | CANNOT ACTIVATE |
| `kNoOutgoingWorld` | NOTHING - NO WORLD IS ACTIVE | NOTHING - NO WORLD IS ACTIVE |
| `kOutgoingNoContainer` | NO CONTAINER - NOTHING TO BACK UP | NOTHING TO BACK UP **←** |
| `kOutgoingEmpty` | LEFT ALONE - CONTAINER IS EMPTY | NOTHING TO BACK UP - SAVE IS EMPTY **←** |
| `kOutgoingNowhere` | BACKED UP - FILED NOWHERE | BACKED UP - NO WORLD TO SAVE IT TO **←** |
| `kOutgoingFiled` | BACKED UP AND FILED INTO IT | BACKED UP AND SAVED TO IT **←** |
| `(already active)` | THIS WORLD IS ALREADY ACTIVE - ITS SAVE STAYS WHERE IT IS | UNCHANGED - THIS WORLD IS ALREADY ACTIVE **←** |
| `(restore)` | THIS WORLD'S OWN SAVE IS PUT BACK | THIS WORLD'S SAVE IS RESTORED **←** |
| `(adopt)` | THIS WORLD HAS NO SAVE YET - IT ADOPTS THE ONE ON THE CONSOLE | ADOPTS YOUR CURRENT SAVE - THIS WORLD HAS NONE **←** |
| `(start fresh)` | THE SAVE IS BACKED UP AND THE GAME STARTS A NEW PLAYTHROUGH | BACKED UP, THEN A NEW PLAYTHROUGH BEGINS **←** |

---

# 8. Activation refusals

*Two of these say the same thing in different words today; the proposal
makes them one sentence, which is also the pane note in §4.*

| Case | Now | Proposed |
| --- | --- | --- |
| save title, zero | NO BLOODBORNE SAVE DATA FOR THIS PLAYER - RUN BLOODBORNE ONCE FIRST | RUN BLOODBORNE ONCE BEFORE ACTIVATING THIS WORLD **←** |
| ownership | THIS WORLD BELONGS TO ANOTHER ACCOUNT | THIS WORLD BELONGS TO ANOTHER ACCOUNT |
| container absent | THIS WORLD HAS A SAVE AND THERE IS NO CONTAINER - RUN THE GAME ONCE | RUN BLOODBORNE ONCE BEFORE ACTIVATING THIS WORLD **←** |
| no signed-in user | NO SIGNED-IN PLAYER - THE ACTIVATION CANNOT BE FINISHED | NO PLAYER SIGNED IN **←** |

---

# 9. Before and after

| | Help strings | Total lines | Total words | Longest |
| --- | --- | --- | --- | --- |
| Now | 26 | 139 | 685 | 10 lines |
| **Proposed** | 26 | **77** | **324** | **6 lines** |

Against a ceiling of 11 lines, the longest is now 6 and most are 1 or 2.
Only `SAVE DATA` reaches 6, and it is the one setting that can destroy
something. `DO NOT RANDOMIZE CAGED DOGS` and `TITLE ID` are 5; everything
else is 4 or fewer.

