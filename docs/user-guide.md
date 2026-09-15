# Bloodborne Randomizer — User Guide

What every setting does, in plain terms. Covers what the app does **today**;
settings that are planned but not built are not listed here.

---

## How it works, in one paragraph

The app never edits your game. It reads a clean copy of Bloodborne's files from
`/data/bbrandomizer/VanillaSource/dvdroot_ps4`, builds a modified copy into
`/data/GoldHEN/AFR/<title id>/dvdroot_ps4`, and AFR layers that copy over the
real game at launch. Every run rebuilds that folder from the clean source, so
settings never pile up on top of each other — what you see on the settings
screen is exactly what you get.

## The menu

| Item | What it does |
|---|---|
| **ENABLE RANDOMIZER** | The wizard: pick a seed and settings, then commit. This is where runs are made |
| **DISABLE RANDOMIZER** | **Not built yet.** To go back to the normal game, delete the `/data/GoldHEN/AFR/<title id>/` folder yourself |
| **SETUP DEFAULTS** | Sets what a *new* wizard run starts from. Editing here changes nothing until you run the wizard |
| **EXIT** | Closes the app |

Both settings screens use the same controls: **left/right or X** toggles the
highlighted row, **O** goes back. In Setup Defaults, **OPTIONS** saves — X does
not. Two rows are drill-ins rather than toggles — the title ID and the enemy
list — and those are opened with **X**.

---

## Settings at a glance

| Setting | Default | Where |
|---|---|---|
| [Bloodborne title ID](#bloodborne-title-id) | `CUSA03173` | Defaults |
| [Seed](#seed) | rolled | Wizard |
| [Backup existing save](#save-data-settings) | Yes | Both |
| [Replace save](#save-data-settings) | New save data | Both |
| [Randomize enemies](#randomize-enemies) | No | Both |
| [Randomize bosses](#randomize-bosses) | No | Both |
| [Randomize treasure](#randomize-treasure) | No | Both |
| [Randomize workshop tools](#randomize-workshop-tools) | No | Both |
| [Randomize enemy drops](#randomize-enemy-drops) | No | Both |
| [Randomize starting weapons](#randomize-starting-weapons) | No | Both |
| [Randomize starting guns](#randomize-starting-guns) | No | Both |
| [Randomize shop weapons](#randomize-shop-weapons) | No | Both |
| [Enable Mergo darkness](#enable-mergo-darkness) | No | Both |
| [Enemies included](#enemies-included) | All 82 | Both |
| [Bosses included](#bosses-included) | All 17 | Both |

Listed in screen order. Every randomizer setting defaults to **No** and the enemy
list starts with everything included, so a fresh install with nothing turned on
produces the normal game.

---

## Setup

### Bloodborne title ID

Which installed copy of Bloodborne the randomized files are built for. **Get it
wrong and the game simply launches unmodified** — AFR will be layering files
onto a title that isn't running, and nothing reports an error. A run that
"did nothing" is almost always this.

The default is `CUSA03173` (Europe / GOTY). Check it matches your copy:

| Title ID | Region / edition |
|---|---|
| `CUSA00900` | USA |
| `CUSA00207` | Australia |
| `CUSA00208` | United Kingdom |
| `CUSA01363` | Asia |
| `CUSA03014` | Japan / The Old Hunters |
| `CUSA03173` | Europe / GOTY |

> If you used a build from before 2026-09-15, the default was `CUSA03175` —
> not a real Bloodborne title ID. That value is saved in `defaults.cfg` and
> **updating the app will not correct it**, because a stored setting always wins
> over the built-in default. Check this row once after updating.

*Setup Defaults only.*

### Seed

The number the shuffle is generated from. **The same seed with the same
settings always produces the same run**, so a seed is how you replay a run or
hand one to someone else. Left/right rolls a new one, X types one in digit by
digit. The seed you last committed is remembered, so repeating a run is just
open-the-wizard-and-commit.

*Wizard only.*

---

## Save data settings

> ⚠ **Not functional yet.** Both of these are wired into the UI and reported in
> the progress log, but nothing touches your save data. The log says
> `(SIMULATED)` where that is the case. **Back your save up yourself before
> starting a randomized run.**

**Backup existing save** — will keep a copy of your current save before a run.

**Replace save** — will choose between starting on new save data, keeping the
save you have, or restoring one of the app's own earlier backups. The backup
entries in that list are placeholders. In Setup Defaults this is **Default
replace save**, and it only pre-fills what the wizard opens on.

---

## World

### Randomize enemies

Replaces the ordinary enemies in the world with random ones drawn from
everything the game has.

Two things keep it playable, and both mean **not every enemy changes**:

- **Each area has its own chance.** Most are 100%, but the Research Hall is 30%,
  Upper Cathedral Ward and Cainhurst Castle are 60%, and the Hunter's Nightmare
  is 90%.
- **Replacements have to fit.** An enemy can only be swapped for one of roughly
  similar physical size, so nothing ends up wedged in a corridor it cannot move
  through.

Bosses are not part of this pool — they have their own setting. You can also
choose which creatures are allowed to appear: see
[Enemies included](#enemies-included).

### Enemies included

A list of all **82 creatures** the randomizer can use as replacements. Everything
is ticked by default, which is the normal behaviour. Untick a creature and it
will never be used.

**Unticking does not protect that enemy — it stops it being used.** Untick
Carrion Crow and you are saying *"never turn anything into a Carrion Crow"*. The
crows already in the world are still placements like any other, and will still be
replaced by something else.

Leave only one ticked and every randomized enemy in the game becomes that
creature.

| In the picker | |
|---|---|
| Up / Down | Move one row |
| L1 / R1 | Page back / forward — 7 pages of 12 |
| X | Turn the highlighted creature on or off |
| Square | Turn everything on |
| Triangle | Turn everything off |
| O | Back |

Square and Triangle both ask before wiping your list, and tell you what you are
about to replace.

Two things worth knowing:

- **Unticking is not evenly weighted.** Some creatures appear in the pool many
  more times than others — Huntsman (Transformed) alone is 12% of every roll,
  while 29 of the 82 are worth 0.3% each. Unticking the Huntsman changes a run
  noticeably; unticking a dozen rare things may be hard to notice.
- **A very small selection weakens the size guard.** If nothing you leave ticked
  is small enough for a given spot, the randomizer eventually places an oversized
  enemy anyway. It will not break the run, but expect some odd fits.

If you turn everything off while `RANDOMIZE ENEMIES` is on, the commit will
refuse to start and tell you to select at least one. Nothing is written.

### Randomize bosses

Shuffles which boss waits in which arena. Every boss is **rescaled for the area
it lands in**, so a first-hour boss appearing late is not a pushover, and a late
boss appearing early is not an instant wall.

You can also choose which bosses are allowed to appear: see
[Bosses included](#bosses-included).

### Bosses included

A list of the **17 bosses** the randomizer can place in an arena. Everything is
ticked by default. It works exactly like
[Enemies included](#enemies-included) — same controls, same Square/Triangle,
same confirmation — just a much shorter list, two pages instead of seven.

Leave only Ludwig ticked and **every boss arena in the game is Ludwig**.

| The 17 |
|---|
| Amygdala · Blood Starved Beast · Cleric Beast · Darkbeast Paarl · Ebrietas · Father Gascoigne · Gascoigne (Beast) · Gehrman · Lady Maria · Laurence · Ludwig · Martyr Logarius · Mergo's Wet Nurse · Moon Presence · Rom · Standing Orphan of Kos · Vicar Amelia |

**Some bosses are not on the list, and cannot be.** Celestial Emissary, the
Witch of Hemwick, Shadows of Yharnam and a few others are excluded from the
pool by the original randomizer's own rules, so they are never used as a
replacement anywhere. With `RANDOMIZE BOSSES` on you simply never meet them —
their arenas get someone from the 17 instead.

**Unticking a boss does not protect its arena.** Untick Ludwig and you are
saying *"never put Ludwig in an arena"* — his own arena still gets somebody
else. This is the same rule as the enemy list.

**Blood Starved Beast and Father Gascoigne appear on both lists** — and they
are genuinely two different creatures each. The game has a boss Gascoigne and a
weaker NPC Gascoigne, and a boss Blood Starved Beast plus a regular one in the
Hunter's Nightmare. The boss list controls the boss versions; the enemy list
controls the others. Unticking one does nothing to the other.

If you turn every boss off while `RANDOMIZE BOSSES` is on, the commit refuses
to start and tells you to select at least one. Nothing is written.

---

## Items

### Randomize treasure

Shuffles the world's pickups: chests, corpses, and items lying on the ground.

It is a **reshuffle, not a re-roll** — the same set of items is dealt back out
across the same set of locations, so everything in the game still exists
somewhere. **Key items are protected** and stay where they are, so a run cannot
become unfinishable.

### Randomize workshop tools

Only does anything with [Randomize treasure](#randomize-treasure) on.

Two permanent unlocks — the **Blood Gem Workshop Tool** (lets you fit gems) and
the **Rune Workshop Tool** (lets you equip runes) — normally sit out of the
shuffle in their usual chests. Turning this on puts them in with everything
else.

Neither is needed to finish the game, so this cannot strand a run. It does mean
you might not be able to use gems or runes until very late, or at all.

### Randomize enemy drops

Changes what each **type** of enemy drops when it dies.

Unlike treasure, this is a re-roll rather than a reshuffle: the same drop can be
assigned to several different enemies, and some drops will not be assigned to
anything. Only the main drop is changed.

---

## Weapons

### Randomize starting weapons

Randomizes the **three melee choices** in the Hunter's Dream. You always get
three different weapons, and their stat requirements are lowered so a level-4
character can actually use whatever turns up.

One side effect worth knowing: there is only one copy of each weapon's stats in
the game, so a weapon that becomes a starting option **keeps those lowered
requirements everywhere**, including the copy you find later.

### Randomize starting guns

The same for the **two firearm choices**. Independent of the melee setting —
have either, both, or neither.

### Randomize shop weapons

Shuffles the weapons the Messengers sell. Like treasure it is a reshuffle, so
every weapon the shop stocked is still stocked, just in different slots.

**It deliberately leaves the five starting slots alone**, so it never overrides
the two settings above. All three can be on at once without interfering.

---

## Other

### Enable Mergo darkness

Mergo's Wet Nurse darkens the arena when you fight her. This turns that darkness
on for the **whole game**.

| | |
|---|---|
| **No** (default) | The normal game, normally lit |
| **Yes** | The world is dark from the moment you spawn, and stays dark |

With it on, loading a new game looks like standing in the middle of the Wet
Nurse fight after she blacks out the arena — and it never wears off. With it
off you get the normal game, which is what a run does unless you ask otherwise.

It is a novelty rather than a difficulty setting: enemies, damage and drops are
all unchanged. It just makes the game very hard to see. Expect to lean on the
lamp posts and on sound.

This is the only setting that is not a randomizer. It changes nothing else, and
the same seed gives the same world whichever way it is set.

---

## What is never randomized

- **Chalice dungeons** — untouched, deliberately.
- **Key items** — always stay in their normal places, so no run can become
  unfinishable.
- **Shop armour and consumables** — only weapons are shuffled today.
- **NPCs** — hostile NPCs are left as they are.

---

## A first run

If you just want to see what this does:

1. Back up your save yourself (the app cannot do it yet).
2. **ENABLE RANDOMIZER** → turn on **Randomize enemies** and **Randomize
   bosses**.
3. Leave the rolled seed alone, or write it down if you want to replay it.
4. Commit, wait for the progress screen to finish, and launch Bloodborne.

Add treasure and weapon settings once you know you like it. A run takes roughly
10–20 seconds to build.
