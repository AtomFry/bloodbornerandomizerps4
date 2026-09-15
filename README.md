# Bloodborne Randomizer for PS4

An enemy and item randomizer for Bloodborne that **runs on the PS4 itself**. No
PC, no file transfers between runs — set your options on the console, press
commit, and launch the game.

It is a native C++ homebrew app (OpenOrbis toolchain, SDL2) that reads a clean
copy of Bloodborne's data, builds a randomized copy, and writes it where
GoldHEN's AFR overlays it onto the installed game. **Your game files are never
edited.** Every run rebuilds from the clean source, so settings never stack on
top of each other.

This is a port of [Alo81's Bloodborne Enemy and Item
Randomizer](https://www.nexusmods.com/bloodborne/mods/4), which runs on Windows
and requires moving the whole game folder back and forth. See
[Credits](#credits).

---

## Status

**Working and hardware-tested** — enemy randomization, boss randomization with
per-zone scaling, treasure shuffling, enemy drops, starting and shop weapons,
workshop tools, Mergo darkness, a reproducible seed, and drill-in pickers for
exactly which enemies and bosses stay in the pool.

> ⚠ **Save data backup and restore are not implemented.** The UI shows those
> options and the progress log prints `(SIMULATED)` next to them, but nothing
> touches your save. **Back your save up yourself before a randomized run.**
> This is the one place the app currently claims to do something it does not.

About twenty further settings from the reference tool are not ported yet —
[docs/randomization-feature-spec.md](docs/randomization-feature-spec.md) tracks
every one with its status. Chalice dungeons are deliberately out of scope.

---

## Requirements

- A **jailbroken PS4 running GoldHEN** with **AFR** (Auto File Replacement)
  installed. This app does not jailbreak anything and cannot — it needs nothing
  beyond ordinary homebrew filesystem access.
- **Bloodborne installed.**
- **A clean, unmodified copy of Bloodborne's `dvdroot_ps4`** that you can put on
  the console over FTP, once. See [First-time setup](#first-time-setup).
- An FTP client. GoldHEN's FTP server listens on port **2121**.

---

## Install

1. Copy the `.pkg` to `/data/pkg/` over FTP.
2. Install it — Settings → Debug Settings → Game → Package Installer, or
   GoldHEN's own installer.
3. It appears on the home screen as **Bloodborne Randomizer**.

---

## First-time setup

**The app needs its own clean copy of the game's data, and you have to provide
it.** Installed game content lives inside an encrypted image that homebrew
cannot read, so the randomizer cannot get a vanilla baseline by itself.

Put a clean `dvdroot_ps4` here, over FTP:

```
/data/bbrandomizer/VanillaSource/dvdroot_ps4
```

> ⚠ **It must genuinely be vanilla.** If you have ever run the Windows
> randomizer against a folder, that folder's live files are its *output* and the
> real vanilla bytes are in the `.bak` files beside them. Seeding from a
> contaminated tree produces a randomization of a randomization — it looks like
> a bug in this app and is not one. **A folder containing `.bak` files is not
> vanilla.**

Then check the **title ID** on the Setup Defaults screen matches your installed
copy of Bloodborne. Get it wrong and the game just launches unmodified, because
AFR will be layering files onto a title that is not running.

| Title ID | Region / edition |
|---|---|
| `CUSA00900` | USA |
| `CUSA00207` | Australia |
| `CUSA00208` | United Kingdom |
| `CUSA01363` | Asia |
| `CUSA03014` | Japan / The Old Hunters |
| `CUSA03173` | Europe / GOTY |

---

## Using it

1. Back up your save yourself.
2. **ENABLE RANDOMIZER** → turn on **Randomize enemies** and **Randomize
   bosses** to start with.
3. Leave the rolled seed, or write it down to replay the run later.
4. Commit, wait for the progress screen, launch Bloodborne.

A run takes roughly 10–20 seconds to build. Every setting defaults to **No**, so
a fresh install with nothing turned on produces the normal game.

**Seeds are reproducible.** The same seed with the same settings always
produces the same world, so runs can be replayed or shared. Changing any toggle
changes the run, even with the same seed — and seeds are not interchangeable
with the Windows tool, which uses a different random number generator.

Full explanation of every setting: [docs/user-guide.md](docs/user-guide.md).

### What is never randomized

Chalice dungeons, key items (so no run can become unfinishable), shop armour and
consumables, and hostile NPCs.

---

## Building from source

Needs the [OpenOrbis PS4 toolchain](https://github.com/OpenOrbis/OpenOrbis-PS4-Toolchain)
with `OO_PS4_TOOLCHAIN` set, plus **lld 18.1.8 specifically** available as
`LLD18_BIN`.

```bash
cd app
make
```

The linker pin is not optional — later lld versions produce an `.oelf` the
console's loader rejects at launch, before `main()` runs. That and every other
hard-won platform detail is in
[docs/ps4-homebrew-findings.md](docs/ps4-homebrew-findings.md).

### Repository layout

| Path | What it is |
|---|---|
| `app/` | The PS4 application — source, Makefile, Python verifiers |
| `docs/` | Feature spec, per-feature plans, platform findings, user guide |
| `reference/` | Alo81's Windows tool, kept read-only as the behavioural reference |
| `data/` | Local game trees (gitignored, not distributed) |

### Testing

There is no host C++ compiler and no emulator in this project's toolchain, so
`app/tools/*_verify.py` scripts mirror each algorithm's rules in Python and check
them against real game data:

```bash
cd app
python tools/pool_verify.py selftest ../data/vanilla/dvdroot_ps4
```

A passing selftest means the *rules* are right, not that the C++ implements them
right. Only a test on real hardware proves a feature works.

---

## Credits

- **[Alo81](https://github.com/Alo81)**, **Sepukkake**, and contributors to the
  original [Bloodborne Enemy and Item
  Randomizer](https://www.nexusmods.com/bloodborne/mods/4) — this port
  reproduces their behaviour, including their deliberate quirks, and that tool's
  source is the specification this was written against.
- **[SoulsFormats](https://github.com/JKAnderson/SoulsFormats)** by JKAnderson —
  the file-format library the Windows tool is built on, and the reference used to
  re-implement MSB and DCX handling natively.
- **[Smithbox](https://github.com/vawser/Smithbox)** by vawser (MIT) — the
  community-curated creature and map name data the PC-side tools print with.
- **Mark Adler's `puff.c`** (public domain, from zlib) — DEFLATE decompression.
- **OpenOrbis** — the PS4 homebrew toolchain.

## A note on scope

This is homebrew for a console that is already modded. It is not a jailbreak, it
contains no exploit, and it needs no privilege beyond what GoldHEN already
grants — that was established by testing rather than assumed, and the app uses
nothing more. No Bloodborne game assets are distributed in this repository.
