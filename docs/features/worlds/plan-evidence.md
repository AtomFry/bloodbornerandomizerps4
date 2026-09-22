# Plan Evidence — Worlds

**Plan:** `docs/features/worlds/plan.md`

**Spec:** `docs/features/worlds/spec.md` (APPROVED 2026-09-21)

**Technical evidence:** `docs/features/worlds/technical-findings.md`

---

> **This document is the investigation behind the plan, not instructions.** The
> implementer consults it when a question needs context the contract left out.
> It is revisable. There is no length budget.

---

## E1. Reference trace

**There is nothing to trace.** The Windows tool in `reference/` has no equivalent
of this feature. It randomizes in place, overwriting the game's files, and has no
concept of more than one configuration, of named playthroughs, or of save data at
all. `reference/Randomizer/MainWindowComponents/` contains no save-data code and
no profile or configuration store; `RandomizeFunctions.cs` writes output directly
to the path the user points it at.

`CLAUDE.md` §7 makes the reference the behavioural default, so the absence matters
in one direction only: **nothing this feature does may change what the reference
produces for a given seed and setting set.** That is plan B28, and it is enforced
by leaving `EnemyRandomizerOptions`, the option mapping in the commit path and the
six mirrored folders exactly as they are. The only change the engine sees is the
output directory string.

The spec agrees: §1 calls this a new feature with no reference-tool equivalent,
and §6 puts "any change to randomization behaviour or output" out of scope. No
contradiction between the spec's §2 and anything in `reference/` was found,
because the two do not overlap.

### E1.1 What was used as a behavioural reference instead

Apollo Save Tool's `exec_cmd.c`, quoted in `technical-findings.md` §4, is the only
prior art for restoring a PS4 save from homebrew:

```c
orbis_SaveMount(save, RDWR | CREATE2 | COPY_ICON, mount);
copy_directory(...);
patch_sfo(<mount>/sce_sys/param.sfo, &patch);   // user_id, account_id, psid
orbis_SaveUmount(mount);                        // unmount re-seals
```

Four deliberate departures, three of them forced by what the platform allows:

1. **No `CREATE2`.** Apollo can create a container because it writes a PFS image
   and a sealedkey under `/user/home` and registers the result in
   `/system_data`'s `savedata.db`. Neither path exists in this sandbox — the
   probe's own directory map shows `/user` and `/system_data` are simply not
   there. The one attempt returned `0x809F0008` and is implicated in corrupting a
   live save (§E5.1), so it is disabled and must not be retried.
2. **No `patch_sfo`, and no writes under `sce_sys` at all.** Apollo rewrites
   ownership because it moves saves between consoles and accounts. This feature
   restores to the same console and account, where `ACCOUNT_ID`, `TITLE_ID` and
   `SAVEDATA_BLOCKS` are already correct — and the attempt would fail anyway: all
   four `sce_sys` entries refuse with `0x8002000D`, EACCES.
3. **No `COPY_ICON`.** That flag copies `save_data.png` out of the *calling
   application's* package. This app's package has no such file, and the icon is
   already in the container.
4. **Empty before writing.** Apollo copies over whatever is there. A container
   holding another world's save carries however many `backup*` files that
   playthrough accumulated, so writing over it leaves the surplus behind — §E5.9.

---

## E2. What exists in the port

### E2.1 Save data — everything is temporary scaffolding

`app/src/Platform/SaveDataProbe.cpp` (693 lines) is the only save-data code in the
app. It is explicitly marked for deletion in its own header. What it already
proves and already contains, which the plan reuses rather than re-derives:

| Piece | Where | Reuse |
| ----- | ----- | ----- |
| `sceUserServiceInitialize` returning `0x80960003` benignly | `ProbeSaveData` | keep; SDL or the system initialises it first |
| BSD flag constants duplicated locally | file-scope | fold into `Platform/SaveData.cpp` |
| `BsdDirent` layout and `d_type == 4` recursion | `Walk::Go` | promote as-is — it is the trap fix |
| Chunked read-through sizing, never `st_size` | `Walk::Go` | promote as-is |
| Short write treated as failure, not retried | `Walk::Go` | promote as-is |
| `SfoString()` — SFO parse validated against real files | file-scope | promote, plus a raw-bytes sibling for the `0x0004` entries |
| The six-SKU candidate sweep | `kCandidates` | promote; it is the discovery mechanism |
| Reconciling the walked total against `sceSaveDataGetMountInfo` | end of probe | promote — it is what catches a silent under-read |
| Backup naming `<title>_<dir>_<stamp>` | `doBackup` block | extend with a `_<reason>` suffix |

`UI/SaveProbeScreen.cpp` (103 lines) is a single-purpose screen with the progress
log's layout (`kLogLayout = {300, 70, 920, 50}`), colouring `FAILED`/`CANNOT` red
and ` OK`/`MOUNTED AT` green. It is deliberately **not** a setting, for the reason
its header gives: `settings_ui_verify.py` asserts the model has exactly the
settings the spec lists, so a fake nineteenth entry would fail that check.

**Decision: extend, do not replace.** Milestone 0 is diagnostic and the scaffolding
is already shaped for it. Milestones 1–3 keep using the same screen as an
operations harness, which is what makes each of them hardware-testable before any
of the new UI exists. It is deleted in milestone 6 (plan §5), and the removal list
is the one its own header already carries.

### E2.2 The AFR side

`Game/AfrManager.{h,cpp}` is 59 lines and already anticipates this feature. Its
header says, of `.bbrandomizer_manifest`:

> Deliberately inside dvdroot_ps4, not somewhere else in the title's AFR folder:
> the eventual transaction model stages/activates dvdroot_ps4 as one atomic unit
> (rename staged -> active), so a marker living inside it travels and disappears
> with the exact content it describes. There is no separate app-level "is it on"
> flag to fall out of sync — nothing writes this file yet.

That is the design the spec's D11 records and the plan's §4.2 and §4.3 implement.
`AfrManager::Check` currently tests only existence; the plan adds a reader, a
writer, and stage/swap/remove helpers, all inside `AfrManager.cpp` so that the
string `/data/GoldHEN/AFR/` stays in exactly one file.

`Game/GameInfo.cpp` detects installs by content fingerprint
(`<entry>/dvdroot_ps4/event/common.emevd.dcx` with the DCX magic), not by a
title-ID whitelist, and can legitimately return more than one. Nothing about this
feature changes that; a world's output still goes to the title Defaults names.

### E2.3 The settings chain

The closest neighbouring feature is `randomizer-settings-ui`, and its chain is
exactly the template this feature follows:

`RandomizerDefaults` (struct) → `RandomizerDefaultsStore` (`key=value` at
`/data/bbrandomizer/defaults.cfg`) → `SettingsModel` (one static table, stable
`SettingId`, pointer-to-member for toggles) → both categorised screens.

Three properties of that chain matter here:

* **Declaration order in `kSettings` is display order.** Adding `SAVE DATA` is one
  entry in one place.
* **`AdjustSetting` is the only writer of a setting.** `SettingKind::SaveChoice`
  therefore needs no new write path — it flips the same bool, and only
  `SettingValueText` changes, to render `KEEP EXISTING`/`START FRESH`.
* **`defaults.cfg` is tolerant.** Unknown keys are ignored and an absent key falls
  back to the struct's own default (`RandomizerDefaultsStore.cpp`). That is why
  the same serializer can write a revision file and why adding `start_fresh_save`
  cannot break an existing install.

`RandomizerDefaultsStore.cpp`'s writer is one `snprintf` with a clamp, and its
comment records that the worst case is 616 bytes against a 1024-byte buffer,
pinned by a `pool_verify.py` selftest case. Extracting it into a shared
serializer must keep that clamp and update that case if the worst case moves.

### E2.4 The seventh category problem

`SettingCategory` has a `Count` member, and both screens size their per-category
cursor arrays from it and iterate `0..Count-1` for the rail. Adding
`SettingCategory::Save` therefore makes `SAVE` appear on **both** screens unless
the iteration changes.

The per-world save policy has no meaning on the Defaults screen: Defaults is what
a *new* world starts from, and a new world's save policy is decided in the editor
where the consequence is visible. So the fix is a per-screen category list rather
than a shared `Count` loop — the editor iterates all seven, Defaults the first
six. `settings_ui_verify.py` case 3 currently asserts the six categories and their
membership against the `randomizer-settings-ui` spec §7.1; it must learn that a
seventh exists, is editor-only, and holds exactly one setting.

Rejected: keeping the save policy out of `SettingsModel` entirely and drawing it
as a bespoke row. That reintroduces exactly the hardcoded-row design
`SettingsModel.h` was written to remove.

### E2.5 Verifiers that name source files

Three parses would break silently on the `EnableWizardScreen` rename:

```
app/tools/pool_verify.py:533        named_strings(os.path.join(UI_SRC, "EnableWizardScreen.cpp"))
app/tools/settings_ui_verify.py:403 strip_comments(read(os.path.join(UI, "EnableWizardScreen.h")))
app/tools/settings_ui_verify.py:677 wiz = parse_geometry("EnableWizardScreen.cpp")
```

`pool_verify.py`'s case parses the four progress-log constants out of the file by
name and asserts their wording and their renderability under the `Font8x8`
fallback. `settings_ui_verify.py` parses the wizard's rail structure from the
header and its geometry constants from the `.cpp`, then compares every shared
constant against `SetupDefaultsScreen.cpp`'s twin — that comparison is what stands
between one geometry and two that drift.

`ui_scroll_verify.py` is table-driven (`SCREENS`, line 171) and names screens only
in strings, so it needs new rows rather than a rename.

### E2.6 Resumable jobs

`EnemyRandomizerJob` (`Randomizer/EnemyRandomizer.h`) is the established answer to
"this takes longer than a frame":

> A full run takes 10-20 seconds, which is far too long to spend inside one
> Screen::Update() call … So the run is exposed as a resumable job instead.
> Step() performs one coarse unit of work and returns.

Every long operation this feature adds — a ~27 MB save backup, a ~27 MB restore, a
~78 MB tree generation, and the activation that contains all three — uses that
same `Step`/`Done`/`StatusText`/`Progress`/`Result` shape. The progress log screen
already knows how to drive it and already scrolls and follows the tail.

Note that the existing probe copies 26 MB **inside one `Update()`** and completes,
so a non-stepped copy would work; it is stepped anyway so the player sees progress
and so the activation job has one uniform driving loop.

### E2.7 File primitives

`Randomizer/FileIo.cpp` already has `CopyDirRecursive`, `MakeDirsRecursive`,
`ReadWholeFile` and `WriteWholeFile`, all built on the hardware-proven BSD flag
values, all sizing by read-through. Two notes for reuse:

* `CopyDirRecursive` skips `*.bak` deliberately (the reference tool's own backup
  artifacts). Harmless for world storage; a save contains no `.bak`.
* It is best-effort: it continues past a failed entry and returns false. For a
  safety backup that is not good enough on its own, which is why the plan pairs
  every copy with a manifest verification pass rather than trusting the return.

`sceKernelRename` and `sceKernelRmdir` exist in the SDK (`orbis/libkernel.h` lines
375 and 381) but **no code in this app has ever called either**. That is why
milestone 0 includes a two-line rename test: the whole transaction model rests on
a directory rename being atomic and working at all on this filesystem.

### E2.8 What the rail does with Left/Right today

D19 rests on a fact worth pinning, because the obvious assumption is the opposite.
`randomizer-settings-ui` spec §2's control table, line 74:

| Left / Right | Change the selected setting's value. **Nothing on the rail** |

and its §10 (A2): "Left/Right no longer roll a seed from the rail. The seed changes
only inside its own editor."

The shipped code agrees, in both screens. `EnableWizardScreen::UpdateRail`:

> Left/Right do nothing anywhere on the rail. They used to roll a new seed on row
> 0, which made an irreversible change to the run from a direction press, with no
> confirmation and no undo.

and `SetupDefaultsScreen::UpdateRail`:

> Left/Right do nothing anywhere on the rail, on this screen or the wizard.

So Left/Right on the rail is free on **both** tabs, and binding tabs to it re-binds
nothing and gives no button two meanings on one screen. The pane keeps Left/Right
for values, which is the binding `randomizer-settings-ui` §10 (9.2) settled.

---

---

## E3. Alternatives considered

| Approach | Why rejected |
| -------- | ------------ |
| Store each world's *generated tree* instead of a recipe | ~78 MB per world. Five worlds is 390 MB of duplicated game data on the same partition as `VanillaSource`. D1 settles it; the measurement (§E4 M2) says why it is settled that way |
| Symlink or hard-link a save between worlds | Spec §7.1 forbids it outright: a save with two owners silently changes under whichever world is not being activated. Copying costs 27 MB and has none of the aliasing |
| Record the active world in `defaults.cfg` | A separate flag drifts from disk. `AfrManager.h` already rejected this design in writing, and `MenuScreen.h` records that a persisted on/off flag "was tried and reverted" |
| Swap the save before the AFR tree | Both orders have an unsafe window, so the journal is what actually fixes it. Save-last is chosen because a failed save swap is recoverable from a backup verified moments earlier, whereas a half-written tree is recoverable only by regenerating — which takes 10–20 s and needs `VanillaSource` present |
| Write the new tree over the live one, no staging | An interrupted write leaves a tree that is neither world's, with a manifest that lies about it. Staging costs one extra ~78 MB directory for the duration of one activation |
| Copy the live save twice: once to the safety backup, once to the world, both from the mount | Two mounts and two full reads of 27 MB. The plan copies mount → safety backup → world, so the bytes that reach the world are the bytes that were verified |
| Per-file CRC32 in the save manifest | Spec §7.1 defines verification as "file count and sizes against a manifest". Adding checksums exceeds the approved definition and costs a second full read. Recorded here because it is the obvious next proposal, and because the *hardware* test (H4) covers byte-exactness by comparing two backups |
| `sceSaveDataDelete` for `START FRESH` | Never needed. Unlinking `userdata*` and `backup*` inside the mount produces a fresh game and leaves a container a later restore can write into; deleting the container would leave nothing to restore into, since the app cannot create one |
| A blank-save template shipped with the app, to seed a container | There is nothing to ship. Bloodborne-save-editor's `saves/emptysave` is 0 bytes and its `saves/newsave` is one slot holding a character called "gggg"; neither is a blank container, and a container is not a file this app can place anyway. Synthesising one would need the save format, and §5 of the findings made the question moot |
| `statfs` in place of `statvfs` for the capacity check | It fills a caller buffer where `statvfs` does not, so it might work — but it is a second unverified ABI on a kernel that has already produced two of them, and D24 removes the need for the check entirely |
| A `COMPLETE` marker file inside a finished backup | Every reader must remember to check it. Writing into `<name>.partial/` and renaming on success makes the check unnecessary rather than merely easy |
| Restoring over a container without emptying it first | The surplus `backup*` files of the previous occupant survive into the new world's playthrough — see §E5.9 |
| A scratch save directory, to keep risky operations away from the live save | It does not work, and milestone 0 is how that was found out. The failed `CREATE2` named `BBRRESTORETEST` and `SPRJ0005` is what broke (§E5.1). Naming bounds *writes*, not failures |
| `sceSaveDataMount2` for restore | It has no `titleId` field, so it can only mount the calling application's own save data, which does not exist for this homebrew (`technical-findings.md` §1.3) |
| Read `INSTALL_DIR_SAVEDATA` from the game's `param.sfo` to learn the save title | Authoritative but unreachable: all five candidate paths return ENOENT, not EACCES, because a game's files exist only inside its own sandbox while it runs (`technical-findings.md` §2.1). Discovery by sweep plus a `PARAMS` match is the empirical substitute |
| Apollo's privileged route (`/system_data/priv/...`, `orbis_jbc.c`) or Itemzflow's libdumper | Out of bounds per `CLAUDE.md` §1; libdumper is also GPLv3, which would relicense this app (`technical-findings.md` §2.2) |
| `sceImeDialog` for entering a world name | A new library, and `docs/ps4-homebrew-findings.md` §7 records that every new library is a chance to repeat the `sceAppInstUtil` bad-NID failure — a crash at load with no output. The character-by-character editor already exists for the title ID |
| A `ScreenManager` back-stack so each wizard step is a `ScreenId` | `Application.cpp` rebuilds a screen on every switch, so in-progress edits would be destroyed. Both existing multi-step screens use an internal mode enum for exactly this reason |
| Promote the probe screen to a real settings row | `settings_ui_verify.py` asserts the model holds exactly the spec's settings; a diagnostic row would fail it. Its own header records this |
| `L1`/`R1` as the tab-switching binding | Unnecessary. Left/Right on the rail is already unused on both screens (§E2.8), so the spec's own wording works as written and `L1`/`R1` stay free for paging a picker |
| Making the tab strip a focusable region, with Up from the rail's first row moving into it | A third focus target on a screen that has two, and one more place for the cursor to be lost. It exists only to create somewhere for Left/Right to mean "switch tabs", which the rail already provides |
| Letting `START FRESH` persist, and relying on the confirmation screen to warn each time | A warning the player sees on every activation is a warning they stop reading. D17 makes the setting expire instead |
| Reverting `START FRESH` in the editor after the activation job returns | An activation that completes but whose screen is torn down first would leave the world armed. Phase 7 puts the revert inside the journalled transaction |
| Defer `START FRESH` to a later feature | It is spec §6 in-scope, D6 decides its shape, and it turned out to need no new API at all — unlinking files in a mount the app already has open |
| One combined milestone for storage plus UI | The UI would be the only way to test the storage, so a storage bug would surface as a UI bug. Driving milestones 1–3 from the existing harness keeps each layer falsifiable on its own |

---

## E4. Measurements

All commands run from the repository root unless stated. `data/` is gitignored, so
a verifier that wants these fixtures must degrade gracefully when they are absent.

| # | Quantity | Value | How measured | Source |
| - | -------- | ----: | ------------ | ------ |
| M1 | Live save content bytes | 26,949,914 (26,318 KB) | `python -c "import os; print(sum(os.path.getsize(os.path.join(r,f)) for r,d,fs in os.walk('.') for f in fs))"` in the backup directory | `data/Save Backups/CUSA00207_SPRJ0005_20260921-032915` |
| M1b | Files in the save | 26 (11 `userdata*`, 11 `backup*`, 4 in `sce_sys`) | `find . -type f \| wc -l` in the same directory | same |
| M1c | Save container allocation | 1136 blocks × 32,768 = 37,224,448 bytes | `SAVEDATA_BLOCKS` read at file offset 2544 of `sce_sys/param.sfo`, format `0x0004`, little-endian u64 | same |
| M2 | Mirrored AFR tree | 81,106,009 bytes (77.35 MiB), 102 files | `du -sb data/vanilla/dvdroot_ps4/{chr,event,map,param,script,sfx}` — the six folders `kMirrorFolders` copies (`EnemyRandomizer.cpp:222`) | `data/vanilla/dvdroot_ps4` |
| M2b | Whole vanilla tree | 81,714,503 bytes | `du -sb data/vanilla/dvdroot_ps4` | same |
| M3 | Save-data title vs game title | saves under `CUSA00207`, game is `CUSA03173` | `TITLE_ID` key = `CUSA00207`; `CUSA03173` found twice inside the `PARAMS` blob at blob offsets 44 and 60 by `re.finditer(rb'CUSA[0-9]{5}', params)` | `sce_sys/param.sfo` of the backup |
| M3b | `PARAMS` blob | format `0x0004`, 1024 bytes, file offset 1520; 93 non-zero bytes | SFO index walk over the 12 keys | same |
| M4 | Owning account id | `0xXXXXXXXXXXXXX604` | `ACCOUNT_ID`, format `0x0004`, 8 bytes at file offset 348 | same |
| M4b | SFO keys present | 12; **no `CONTENT_ID`** | full index walk | same |
| M5 | World-name budget | 16 characters | `settings_ui_verify.load_atlas()`; `'W'` advances 30 px at scale 3, so 16 worst-case glyphs are 480 px inside the 580 px rail, leaving 100 px for the gap and the active marker | `app/src/Platform/FontAtlasData.h` |
| M5b | `SAVE DATA` + `KEEP EXISTING` at scale 3 | 179 + 237 = 416 px | same loader, `width()` | pane is 700 px wide (`kPaneW`) |
| M5c | Tab labels at scale 4 | `WORLDS` 184 px, `DEFAULTS` 221 px | same | screen is 1920 px |
| M5d | Widest shipped rail label at scale 3 | `WEAPONS & STARTING GEAR`, 458 px | same | rail is 580 px |
| M6 | Peak activation disk cost | ~78 MB staging tree + ~27 MB safety backup + ~27 MB world save ≈ 132 MB above steady state | M1 + M2 | derived |
| M7 | Steady growth per activation | ~27 MB, never pruned (the safety backup) | M1 | derived |
| M8 | `defaults.cfg` worst case today | 616 bytes against a 1024-byte buffer | recorded in `RandomizerDefaultsStore.cpp` and pinned by a `pool_verify.py` selftest case | `app/src/Randomizer/RandomizerDefaultsStore.cpp` |

### E4.1 How the param.sfo figures were derived

```
python -c "
import struct,re
b=open('sce_sys/param.sfo','rb').read()
kt,dt,cnt=struct.unpack_from('<III',b,8)
for i in range(cnt):
    ko,fmt,vl,ms,do=struct.unpack_from('<HHIII',b,0x14+i*16)
    k=b[kt+ko:b.index(b'\x00',kt+ko)].decode()
    print(k, hex(fmt), 'len',vl,'off',dt+do)
"
```

Run inside `data/Save Backups/CUSA00207_SPRJ0005_20260921-032915`. It reproduces
`technical-findings.md` §3's key list exactly and gives the offsets M1c, M3b and
M4 quote. The `0x0004` entries (`ACCOUNT_ID`, `PARAMS`, `SAVEDATA_BLOCKS`) are not
UTF-8 and are invisible to the probe's existing `SfoString()`, which returns empty
for any format other than `0x0204` — hence the raw-bytes sibling milestone 0 adds.

`SAVEDATA_BLOCKS` matters beyond curiosity: it is the size a container must have
for a stored save to fit, and it is readable **from the backup** as well as from
the container, so the two can be compared before a restore writes anything (D23).
It cannot be changed — `param.sfo` is unwritable — so the container's size is a
fact to check against, never a thing to set.

### E4.2 Free space cannot be measured

The suspicion was a struct-layout mismatch — `statvfs` is declared in
`sys/statvfs.h` with musl's Linux `struct statvfs` on a FreeBSD-derived kernel,
the same shape of problem that makes `OrbisKernelStat::st_size` return 88 for a
44 KB file. The reality is simpler and worse. Disassembling `libc.a(statvfs.lo)`:

```asm
statvfs:  subq $0x928,%rsp ; movq %rsp,%rsi ; callq statfs
          sarl $0x1f,%eax  ; addq $0x928,%rsp ; retq
```

Argument 2 — the caller's buffer — is overwritten with a local stack address
before the call and never copied back. On hardware, `/data` returned rc 0 with
128 raw bytes of zero, and `/user` returned `0xFFFFFFFF`. The layout was never the
question: there is no output at all.

`statfs` does fill a caller-supplied buffer, with FreeBSD's 488-byte struct, and
is untested. It was not pursued. D24 removes the pre-flight check rather than
build a refusal on a second unverified ABI, and the transaction carries the weight
instead by aborting when the backup does not complete.

---

## E5. Risk analysis

### E5.1 A failed save-data operation is not a no-op

The isolation model this plan rested on was wrong, and the way it failed is the
most important single fact carried out of milestone 0.

The assumption was that naming a scratch directory bounded the blast radius: a
write path that never names the live directory cannot damage the live save. The
probe was built that way — a compile-time `BBRRESTORETEST`, and a guard refusing
to proceed if that name ever matched a directory the search returned.

The `CREATE2` mount of `BBRRESTORETEST` failed with `0x809F0008`. It never named
`SPRJ0005`. **`SPRJ0005` is what stopped working**: Bloodborne hung on a black
screen afterwards and recovered only when the save was deleted from the console's
Saved Data Management. The obvious competing explanation — swapping AFR content
under an existing save — was ruled out by the developer, who has done that many
times without incident.

Causation is one correlation, not proof. But the operation cannot succeed and may
disturb the title's save registry, so two assumptions are retired:

* **A failed operation is a no-op.** It is not. The failed `CREATE2` left
  something changed.
* **Directory naming scopes the damage.** It does not. A guard stops a *write* to
  the live directory; it does nothing about a failed *create* elsewhere in the
  same title's save registry.

What the plan does instead: never set `CREATE2` at all (P18), and bracket every
container write with a read of the container's file list and total, before and
after (P22). Bracketing prevents nothing — nothing available does — but it turns
"did that hurt?" into a dated observation, which is the one thing that would have
pinned this incident when it happened.

Note what is *not* dangerous, now that it has been run: an `RDWR` mount of an
existing container is granted and harmless on its own; overwriting files works;
creating files works; unlinking files works. The risk was never read/write access
to a container. It was bringing one into existence.

### E5.2 The backup can run out of disk with no warning

Free space cannot be measured (§E4.2), so the pre-flight refusal D13 assumed is
not buildable. The failure it guarded against is real: a safety backup is ~27 MB,
an activation writes two of them plus a ~78 MB staging tree, and none of it is
pruned.

D24 moves the guard from before to during, and the plan's mechanism is:

* the backup is written into `<name>.partial/`;
* its manifest is written **last**, after every file has copied and verified;
* the directory is then renamed to its final name — `sceKernelRename` on a
  directory is proven;
* any failure aborts the whole transaction, and every `*.partial` is swept at
  startup.

A backup is therefore atomic from a reader's side: either it is a complete
directory with a manifest, or it does not exist. No state exists in which a
partial copy looks like a recovery path.

Rejected: writing a `COMPLETE` marker into the final directory. It needs every
reader to remember to check for it; the rename makes the check unnecessary rather
than merely easy.

### E5.3 Every existing console is in the `UNMANAGED` state

The current build's Enable wizard writes `/data/GoldHEN/AFR/<title>/dvdroot_ps4`
and writes **no** manifest — `AfrManager.h` says so and `MenuScreen.cpp` logs
`randomized=N` for that reason. So any console that has ever run the app has a
seeded AFR tree with no manifest.

If that state were read as "Vanilla active", the app would tell the player the
game is unmodified while it plainly is not, and the first activation would back up
a save produced by an unmanaged randomization into whichever world happened to be
outgoing. Hence plan P2: seeded-without-manifest is `UNMANAGED`, no row shows
`ACTIVE`, and the details pane says randomizer files are present that this app did
not write. Activating anything, including Vanilla, resolves it.

This interacts with first-run capture (B26): on such a console the live save is a
randomized playthrough being captured into Vanilla, which is not literally where
it belongs. Raised to the developer as a spec gap during planning and settled as
spec **D18** — the save still goes to Vanilla, because preserving it matters more
than where it is filed, and the `UNMANAGED` state is what keeps the app from
claiming the game is unmodified. Plan B32 and P16.

### E5.4 `START FRESH` and why it expires

The hazard, before it was closed: `SAVE DATA` lives in the world's recipe, so a
world left on `START FRESH` would still be set to it the next time it was
activated, phase 6 would again decline to restore that world's save, and the next
deactivation would overwrite the stored save with the fresh playthrough. The old
one would then survive only as a safety backup. Nothing immediate breaks, which is
what makes it the worse of the two shapes of failure — it is the "silently
incoherent" case spec §7.1 singles out. Likelihood was high for anyone who used
the setting at all, because nothing in the flow prompts a player to change it back.

Spec **D17** closes it: `KEEP EXISTING` is always the default, and `START FRESH`
applies to one activation and then reverts, recorded as a revision the player can
see in `HISTORY`. Plan §4.3 phase 7 performs the revert inside the same
transaction, so it cannot be skipped by a screen that forgets to do it, and it
cannot half-apply — a run that does not reach commit leaves the policy as the
player set it, which is also what reconciliation will re-run.

Residual risk, judged acceptable: the revert appends a revision the player did not
ask for, so a world used once with `START FRESH` shows two revisions with identical
randomizer settings. They are output-identical (P5 — regeneration ignores
`SAVE DATA`), and the alternative was a setting that silently discards progress.

Rejected while closing it: **reverting in the UI, after the job returns.** An
activation that completes but whose screen is torn down before it writes would
leave the world armed. Putting it in phase 7 makes the revert part of the thing
the journal covers.

### E5.5 Two ~27 MB copies per activation, forever

Every activation writes a safety backup that is never pruned (M7). Fifty
activations is 1.35 GB. The spec forbids auto-deletion and requires usage to be
shown, so the plan shows it and does not prune. Raised in the report as a deferred
gap rather than solved here, because a retention policy is a behavioural decision.

### E5.6 The dependency on `randomizer-settings-ui`

That feature's four milestones are **implemented and never hardware tested**
(`docs/features/README.md`). This feature builds directly on top of three of its
products: `SettingsModel`, the categorised screen, and the removal of the old
save-data handling.

Assessment: **it does not change the sequencing, and it does change what a
milestone-4 or milestone-5 hardware failure means.**

* Milestones 0–3 touch none of it. They run through the probe screen, which
  predates the settings UI and shares nothing with it. If the settings UI turns
  out to be broken on hardware, milestones 0–3 are unaffected.
* Milestones 4–6 are built on screens that have never been drawn on a TV. A
  layout or input defect found then could belong to either feature.
* The mitigation is cheap and already in the plan: `settings_ui_verify.py` and
  `ui_scroll_verify.py` measure both features' geometry from the same atlas, and
  the milestone-4 and milestone-5 gates run them. If a hardware defect appears in
  a region those tools already assert, it is a tooling gap in the *earlier*
  feature and should be reported as such rather than patched here.
* One concrete pre-existing risk carries over: `Renderer::FillRectBlend` (alpha
  blending) had never run on this hardware when `randomizer-settings-ui` shipped
  its milestone 2. The worlds screens do not depend on it — the active marker is a
  plain `FillRect` and there is no new overlay — so this feature adds no exposure
  to it.

**Recommendation to the developer:** hardware-test `randomizer-settings-ui` before
milestone 4, not before milestone 0. Milestones 0–3 are independently valuable,
independently testable, and answer the questions that could invalidate the whole
design. Blocking them behind an unrelated UI test would delay the gate for no
reduction in risk.

### E5.7 What `KEEP EXISTING` means

Spec §2's `SAVE` table describes `KEEP EXISTING` as "carry the currently live save
into this world". Read literally that describes a *new* world adopting the live
save, and it is the only reading that table supports. For a world that already has
its own stored save, carrying the live save in would overwrite that world's
progress with the outgoing world's — surprising, and a form of the aliasing D4
forbids. It also contradicts spec §2's own activation step 3, "restored from that
world, or started fresh if the player chose that".

The reading that makes both true at once, and the one the plan implements
(§4.3 phase 6, plan B30):

| Incoming world | `KEEP EXISTING` means |
| -------------- | --------------------- |
| Has a stored save | Restore **that world's own** save |
| Has no stored save | Adopt the live save, and copy it into the world |

**A world's stored save is never overwritten with another world's.** The
confirmation screen (B10) must name which of the two applies, so the player sees
the branch rather than inferring it.

This reading was raised to the developer as a spec ambiguity during planning and
accepted. Note for a later reader: the spec's §2 `SAVE` table still carries the
original one-line wording, so §2 and this reading do not read alike on the page.
The binding record is §7.2 and §10.

### E5.8 Activating the already-active world

Re-rolling a world while keeping progress is a flow the spec explicitly wants
(§2, Revisions: "the practical fix for an impassable area"). It makes outgoing and
incoming the same world, and a naive transaction would back the save up, then
restore the copy it just made — 54 MB of pointless I/O and a window in which the
live save is deleted and rewritten for no reason.

Phase 6's last row removes it: when `from == to` and the policy is
`KEEP EXISTING`, the save swap does nothing at all. Phases 2 and 3 have already
captured the live save, and the live save is already the right one.

### E5.9 A world's save carries its own `backup*` count

`backup*` files accumulate as a playthrough runs. The findings measured a freshly
created container holding only `backup0000`, `backup0001` and `backup0010`, and a
played one holding all eleven. All eleven `userdata` slots exist from the moment
the container is created, so the variable part is the backups.

That makes a naive restore leak state between worlds. Restore world B — three
`backup*` files — into a container currently holding world A's eleven, and eight
of A's remain. They are not corrupt; they are A's rolling backups sitting in B's
playthrough, which is the aliasing D4 forbids arriving by a route D4 did not
anticipate. Whether the game would ever read them is unknown and beside the
point: two worlds would be sharing files, which is the thing the design says
cannot happen.

Emptying first (P20) removes it entirely, and costs nothing new — it is the same
operation `START FRESH` performs, hardware-proven, and the container ends holding
exactly the file set the incoming world last ran with.

---

## E6. What the spec's appendix claimed

The spec has no stage C appendix. It defers every platform fact to
`technical-findings.md`, which is a separate, hardware-confirmed document rather
than an unreviewed research note. Everything in it that this plan depends on was
re-derived from the artifacts it names:

| Finding claim | Re-derived? | How |
| ------------- | ----------- | --- |
| 26 files, 26,949,914 bytes | yes | §E4 M1, M1b, against the backup on disk |
| `SAVEDATA_BLOCKS` = 1136 = 0x470 | yes | §E4 M1c |
| `TITLE_ID` = `CUSA00207`, `SAVEDATA_DIRECTORY` = `SPRJ0005` | yes | §E4 M3 |
| `CUSA03173` appears twice in `PARAMS`, at blob offsets 44 and 60 | yes | §E4 M3, by pattern scan rather than at a fixed offset |
| 12 SFO keys, no `CONTENT_ID` | yes | §E4 M4b |
| `PARAMS` is format `0x0004`, not the `0x0204` `SfoString()` handles | yes | §E4.1 — confirmed, and it is why milestone 0 adds a raw-bytes reader |
| `sceSaveDataMount2` has no `titleId` | yes | `orbis/_types/save_data.h` — `OrbisSaveDataMount2` has `dirName` and no title field; `OrbisSaveDataMount` has `const char *titleId` |
| `-lSceSaveData` links and loads | not re-derivable off-console | taken as given; it is in `LIBS` and the probe ran |
| `RDWR` works, `CREATE2` refused, `sceSaveDataDelete` never needed | yes | milestone 0 answered all three |
| `GetNpAccountId` equals the save's `ACCOUNT_ID` | yes | milestone 0: both printed `0xXXXXXXXXXXXXX604` |

Two additions this plan makes to the findings' unknown list:

* **`sceKernelRename` on a directory** (§E2.7) — untested when this plan was
  written, proven in milestone 0, and now load-bearing twice: the AFR swap and the
  `.partial` backup rename.
* **`statvfs` reports nothing** (§E4.2), which is why D13 was superseded by D24.

---

## E7. Anything that could not be established

* **What the failed `CREATE2` actually did to the save registry.** One
  correlation, no mechanism (§E5.1). The plan treats it as real because the
  operation has no upside — it cannot succeed — so there is nothing to weigh
  against the risk. If it were ever needed, this would have to be understood
  first, and it is a stop condition precisely so that question reaches a human.
* **Whether `statfs` fills a caller buffer where `statvfs` does not.** Untested,
  and deliberately not pursued: D24 removes the need rather than resting a
  refusal on a second unverified ABI.
* **Whether a world's save survives being restored into a container it did not
  come from.** Every hardware run so far restored a container's own save back into
  it. The bytes are the same either way, and `sce_sys` — the only part that
  identifies the container — is never written, so there is no mechanism for it to
  matter. But it has not been done, and H6 is the first test that does it.
* **Whether the `backup*` count varying between worlds matters to the game.** The
  findings show a fresh save with three and a played one with eleven, so the game
  clearly tolerates a range. Emptying before a restore (P20) makes the container
  hold exactly the incoming world's set, which is the state that world last ran
  in. Untested across worlds until H6.
* **Whether a second account behaves as D12 assumes.** Single-account console;
  `GetForegroundUser` and `GetInitialUser` are indistinguishable here.
* **Whether any Bloodborne install produces more than one save directory.** The
  findings list it as unknown and the plan refuses rather than guesses. If a
  second directory exists on some install, the refusal is the correct behaviour
  until the developer says what the second one is.
* **Whether the two `CUSA#####` copies in `PARAMS` can disagree.** n=1. The plan
  makes disagreement a stop condition.
* **Throughput, beyond one figure.** Milestone 0 measured **~15 MB in ~1,009 ms**
  for a container write. Nothing is known about AFR write speed, which dominates
  an activation at ~78 MB, so the duration on the confirmation screen (B10) stays
  coarse until H4 and H6 report elapsed times.
* **The editor rail's vertical constants.** The seventh category plus `NAME` and
  `HISTORY` take the rail from 8 rows to 11. The existing constants
  (`kRailRow0Y` 230, `kRailFirstY` 330, `kRailPitch` 76, `kRailRule2Y` 778,
  `kFinishY` 806, footer 1000) do not accommodate that unchanged, and the ink-box
  model that decides it lives in `settings_ui_verify.py`. The plan states the rail
  composition and makes the verifier the authority on the numbers rather than
  asserting pixel values that were never rendered. The lever, if it does not fit,
  is `kRailPitch` — it may drop to 68 before the 64 px focus bars touch.
* **Whether `sceSaveDataDirNameSearch` can be made to return a stable order.**
  `OrbisSaveDataDirNameSearchCond` has `key` and `order` fields that the probe
  zeroes. The plan does not rely on ordering at all, so this was not pursued.
