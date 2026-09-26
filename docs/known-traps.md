# Known Traps

This document records failure modes and misleading signals that have already caused wasted investigation time. Check the relevant section before modifying unfamiliar areas.

## Source editing

### Heredoc backslash mangling

Writing C/C++ source through a `python - <<'PYEOF'` heredoc in the Bash tool can consume one level of backslash escaping before Python sees it.

This has previously:

* inserted a real NUL byte into `EnableWizardScreen.cpp`
* caused `grep` to report the file as binary
* removed newline escapes from `printf` format strings in `RandomizerDefaultsStore.cpp`

**Rule:** Never write a C string or character literal containing a backslash escape through a heredoc.

Use the Edit tool instead. If a bulk scripted edit is genuinely necessary, operate on bytes and construct the backslash with `bytes([92])` rather than typing a backslash in the script.

## UI and fonts

### 8x8 font limitations

The 8x8 font currently supports uppercase A–Z, digits, space, and five punctuation characters:

`'` `(` `)` `-` `,`

`FindGlyph` renders unsupported characters as blank rather than reporting an error.

The glyph table in `app/src/Platform/Font8x8.cpp` is authoritative. Read it before assuming a character is unsupported, and check new user-facing strings against it.

Adding a glyph changes platform functionality. Propose the change explicitly rather than adding it incidentally to unrelated work.

The five punctuation glyphs were added to support enemy names such as:

* `Mergo's Attendant`
* `Huntsman (Transformed)`
* `Shark-Giant`

## Game-data trees

### `.bak` files in vanilla trees

A `.bak` file inside a supposedly vanilla tree indicates that the live files have been modified.

The Windows reference tool creates randomized output in the live file and preserves the original vanilla bytes in the `.bak` sibling.

A tree that has been used by the reference tool should therefore not automatically be treated as a vanilla source.

This previously contaminated the vanilla source and caused an extended investigation of a bug that did not exist.

Verify the actual file contents before using a tree as a vanilla baseline.

## Parameter interpretation

### Large `NPCParamID` values

The 900,000,000+ `NPCParamID` values produced by the reference tool are not automatically corruption.

They can represent intentional hand-curated scaling data from `NPCScalingFile.txt`.

Do not classify parameter values as corrupt based only on map-placement checks. Confirm that the values are invalid parameter rows before drawing that conclusion.

### Byte decoding does not prove game behavior

Knowing which value or flag is written does not establish what the game does with that value.

Do not rename a setting, invert its polarity, or infer its runtime meaning from decoded bytes alone. Verify the behavior on hardware.

## PS4 filesystem APIs

### `OrbisKernelStat.st_size`

`OrbisKernelStat.st_size` has produced incorrect file sizes on this SDK. On real hardware, a genuine 44 KB file returned a reported size of 88.

`ReadWholeFile` therefore does not use `st_size` to determine the file size.

The kernel structure's layout should not be assumed to match the real ABI field-for-field. Treat any `OrbisKernelStat` field that has not already been validated on hardware with suspicion.

`st_mode` has been hardware-validated and has behaved correctly.

## An incremental `make` did not rebuild on a changed header

**Fixed 2026-09-26**, recorded because the symptom pointed nowhere near the
cause and the next person to see something like it should recognise the shape.

`app/Makefile`'s compile rule was `$(INTDIR)/%.o: $(PROJDIR)/%.cpp` and nothing
else, so an object's only prerequisite was its own `.cpp`. **A changed header
rebuilt nothing.**

It surfaced when the startup-screen work changed `UI/WorldsScreen.h` — removing
two members, adding four — without touching `Application.cpp`, which includes
that header and calls `make_unique<WorldsScreen>()`. An incremental `make`
rebuilt `WorldsScreen.o` and left `Application.o` eight hours stale. The
allocation therefore used the **old** `sizeof` while the constructor wrote the
**new** layout past the end of it.

**The symptom was not a crash at the corruption.** The app started, ran, drew
the rail and the details pane. It died at the first screen switch — the first
free-and-allocate after the overflow — with a log that ended on a line that had
nothing to do with the fault. Nothing in the source was wrong.

`CXXFLAGS += -MMD -MP` and `-include $(DEPS)` now generate and consume real
header dependencies. Two things follow:

* **A crash that appears after an incremental build, in code you did not
  change, is a stale-object suspect first.** Compare object mtimes against
  header mtimes before reading the code.
* **Milestone 6 of the worlds feature rebuilt with `rm -rf src/x64 && make`**,
  which is why the hardware test that passed on 2026-09-25 was sound. Builds
  that used a plain `make` after a header change were not.
