# Save data — technical findings

**Status: hardware-confirmed** unless a line says otherwise. Every result below
was produced by the temporary probe in `app/src/Platform/SaveDataProbe.cpp` on a
real PS4 between 2026-09-20 and 2026-09-22, not inferred.

This is the evidence behind `docs/features/worlds/spec.md`. The spec states what
the feature does; this states what the platform actually permits, and how we
know. Findings that outlive this feature belong in
`docs/ps4-homebrew-findings.md` once the feature ships.

---

## 1. Summary — what the platform permits

| Operation | Verdict |
| --- | --- |
| Mount another title's save `RDONLY` | **Works** |
| Copy the save out (backup) | **Works**, byte-exact |
| Mount an existing container `RDWR` | **Works**, and harmless on its own |
| Overwrite an existing file in the mount | **Works** |
| **Create** a file in the mount | **Works** |
| **Unlink** a file in the mount | **Works** |
| Write anything under `sce_sys/` | **Refused — EACCES** |
| **Create a save container** (`CREATE2`) | **Refused**, and implicated in corrupting a save |
| `statvfs` for free space | **Returns nothing** — the libc entry point is a stub |
| `sceKernelRename` on a directory | **Works** |
| `sceSaveDataDelete` | **Never called. Not needed** — see §5 |

Everything the Worlds feature needs is available. Nothing needs privilege
escalation, sandbox escape, or libjbc — `docs/ps4-homebrew-findings.md` §7's
"deliberately not used" stands.

---

## 2. Reading and backing up

The supported route is the official SDK, and it is sufficient:

```
sceUserServiceGetForegroundUser()  → which user's saves
sceSaveDataInitialize3()           → start the service
sceSaveDataDirNameSearch()         → enumerate a title's save directories
sceSaveDataMount()  RDONLY         → mount one, yielding /savedataN
   ordinary sceKernelOpen / sceKernelGetdents / sceKernelRead
sceSaveDataUmount()
```

**The mount is what does the work.** The returned path is a decrypted view that
ordinary file calls walk. No key handling, no decryption.

Verified byte-exact: 26 files, **26,949,914 bytes**, matching the probe's
reported total and the copy pulled to a PC. A read-only run followed by a game
launch confirmed reading is harmless.

### 2.1 Linking libSceSaveData is safe

`-lSceSaveData` was the one new library needed, and §7 records that each new
library risks the `sceAppInstUtilInitialize` bad-NID failure — binary rejected
before `main()`, no output. **It links and loads.**

`sceUserServiceInitialize()` returns `0x80960003` (already initialized — SDL or
the system got there first). Benign: a valid user id follows immediately.

### 2.2 Identity

Every save search is scoped by `cond.userId`, so the app only ever sees one
user's save data. Confirmed working:

```
GETFOREGROUNDUSER 0x00000000 ID 323809701
GETUSERNAME       0x00000000 <psn name>
GETNPACCOUNTID    0x00000000 0xXXXXXXXXXXXXX604
PARAM.SFO ACCOUNT_ID         0xXXXXXXXXXXXXX604   ← MATCH
```

**`sceUserServiceGetNpAccountId` matches the `ACCOUNT_ID` inside the save's own
`param.sfo`.** Ownership can be checked without parsing the save. This settles
plan **P14**.

`GetForegroundUser` and `GetInitialUser` returned the same id on this
single-account console, so they are still indistinguishable here. Prefer
`GetForegroundUser`.

### 2.3 Use sceSaveDataMount, not Mount2

`OrbisSaveDataMount2` has **no `titleId` field** — it can only mount the calling
app's own save data. Only the original `OrbisSaveDataMount` carries
`const char *titleId`.

---

## 3. The save-data title ID is not the application title ID

The console's game is **CUSA03173** (Europe/GOTY). Its save data lives under
**CUSA00207** (Australia). The game's own `param.sfo` carries
**`INSTALL_DIR_SAVEDATA = CUSA00207`**, the official mechanism letting regional
SKUs share one save container. The save directory is `SPRJ0005`, FromSoftware's
internal name.

**The save feature must never reuse the AFR title ID.** A single search against
the configured title returned zero hits on a console that plainly had saves, and
looked exactly like a broken mechanism.

### 3.1 We cannot read the game's param.sfo

All five candidate paths returned `0x80020002` — **ENOENT, not EACCES**. We were
not refused; the paths do not exist in our sandbox. Mapping what is visible:

```
/            OPEN OK  - app0 dev system_tmp data host hostapp ... av_contents
/app0        OPEN OK  - assets sce_module sce_sys eboot.bin
/data        OPEN OK  - 24 entries
/user        NOT THERE     /user/app     NOT THERE     /user/home  NOT THERE
/system_data NOT THERE     /mnt          NOT THERE     /mnt/sandbox NOT THERE
```

A game's files only appear at `/mnt/sandbox/pfsmnt/<TITLE>-app0/` **while that
title is running**, and this app runs instead of the game.

**The empirical sweep is therefore the only discovery route**, and it works: it
found `CUSA00207 / SPRJ0005` unambiguously on every run.

### 3.2 Routes deliberately not taken

* **Apollo Save Tool** reads `/system_data/priv/mms/app.db` and
  `savedata.db` via its `orbis_jbc.c`. Both paths are absent here. Its `CREATE2`
  path *manually builds a PFS image and a sealedkey*, then registers the result
  in SQLite — which is why it can create containers and we cannot.
* **Itemzflow's libdumper** assumes the target title is *currently running*,
  reads `/system_data/priv/`, needs PFS mounting and SELF decryption, and is
  **GPLv3**.
* **Bloodborne-save-editor** (GPL-3.0) anchors every offset on the character's
  username, so it assumes a character exists. Its `saves/emptysave` is 0 bytes
  and `saves/newsave` is one 1,310,720-byte slot containing a character named
  "gggg" — neither is a blank-container template. Useful only if we ever want to
  *read* character name / level / playtime for display.
* **Decompiling `orbis-pub-sfo.exe`** solves nothing: SFO parsing is ~40 lines of
  public format and is already implemented. Parsing was never the obstacle.

---

## 4. Container layout and the write rules

A Bloodborne save container holds:

| Group | Count | Size each | Writable? |
| --- | --- | --- | --- |
| `userdata0000`–`userdata0009` | 10 | 1,310,720 | **yes** |
| `userdata0010` | 1 | 262,144 | **yes** |
| `backup0000`–`backup0010` | 0–11 | mirrors the above | **yes** |
| `sce_sys/param.sfo` | 1 | 2,728 | **no — EACCES** |
| `sce_sys/sce_paramsfo1` | 1 | 32,768 | **no — EACCES** |
| `sce_sys/sce_icon0png1` | 1 | 116,736 | **no — EACCES** |
| `sce_sys/icon0.png` | 1 | 58,994 | **no — EACCES** |

`backup*` files accumulate as the game plays: a freshly created save had only
`backup0000`, `backup0001` and `backup0010`, while a played one had all eleven.
All eleven `userdata` slots exist from the moment the game creates the container.

### 4.1 sce_sys is read-only, and that is fine

A full restore reported:

```
sce_sys/param.sfo      - OPEN REFUSED 0x8002000D (OVERWRITE)
sce_sys/sce_paramsfo1  - OPEN REFUSED 0x8002000D (OVERWRITE)
sce_sys/sce_icon0png1  - OPEN REFUSED 0x8002000D (OVERWRITE)
sce_sys/icon0.png      - OPEN REFUSED 0x8002000D (OVERWRITE)
WROTE 13 OF 17 FILE(S)   OVERWROTE 13  CREATED 0  REFUSED 4  SHORT 0
```

`0x8002000D` = **EACCES**. This costs nothing: `sce_sys` holds `ACCOUNT_ID`,
`TITLE_ID`, `SAVEDATA_BLOCKS` and the icon, all of which are **already correct
for the container being written into**. Apollo needs `patch_sfo` only because it
moves saves between accounts and consoles. Same console, same account, same
title — nothing there needs changing.

**The restore file set is: everything except `sce_sys`.** This settles plan
**P15**.

### 4.2 Consequence — the container's size is fixed

`SAVEDATA_BLOCKS` lives in `param.sfo`, which cannot be written. A save can only
be restored into a container **at least as large** as it needs. Both were 1136
blocks (36,352 KB) here, so it did not bite — but a size check belongs before a
restore rather than being discovered partway through.

Throughput: **~15 MB in ~1,009 ms.**

---

## 5. START FRESH needs no new API

Deleting the game's own files inside the mount is permitted and produces a fresh
game. Two runs, both clean:

```
DELETE backup*    REMOVED 3   REFUSED 0   LEFT ALONE 15
DELETE userdata*  REMOVED 11  REFUSED 0   LEFT ALONE 4
AFTER 4 FILE(S), 211226 BYTES   ← sce_sys only
```

The game then **started a new game**, and a subsequent restore **brought the
saves back**.

That restore is itself a significant result: the `userdata` files no longer
existed, so restoring them was **file creation**, not overwriting — the one
permission the earlier runs had never exercised (`CREATED 0` throughout).

So:

* **`START FRESH`** = unlink `userdata*` and `backup*`, leave `sce_sys`. No
  `sceSaveDataDelete`, no blank-save template, no format knowledge.
* **Restore** works whether or not the target files exist.
* **`sceSaveDataDelete` has never been called** and is not needed. The probe's
  step 7 was gated on the `CREATE2` mount that never succeeded, so it was
  unreachable dead code throughout.

---

## 6. CREATE2 is refused, and is implicated in corrupting a save

```
MOUNTING CUSA00207 / BBRRESTORETEST RDWR|CREATE2
MOUNT FAILED 0x809F0008 - REQUIRED BLOCKS 0
```

This app cannot bring a save container into existence. Apollo does it by writing
a PFS image and a sealedkey under `/user/home` and registering the result in
`/system_data`'s `savedata.db` — neither of which exists in this sandbox.

**After the run in which this failed, Bloodborne hung on a black screen** and
only recovered when the save was deleted from the console's own Saved Data
Management. The obvious competing explanation — swapping AFR content under an
existing save — was ruled out by the developer, who has done that many times
without incident. One anomaly, one novel operation, the same window.

> **The isolation model did not hold.** The failed mount named `BBRRESTORETEST`
> and never named `SPRJ0005`, yet `SPRJ0005` is what stopped working. Naming a
> different directory did not contain the damage. A scratch-name guard prevents
> writing to the live directory; it does not prevent a failed *create* from
> disturbing the title's save registry.

Causation is one correlation, not proof. But the operation cannot succeed and
may corrupt a playthrough, so it is disabled behind `kAttemptCreate2 = false`
and must not be retried.

---

## 7. statvfs cannot report free space

`statvfs` is declared in the toolchain but its libc implementation discards the
caller's buffer. Disassembling `libc.a(statvfs.lo)`:

```asm
statvfs:  subq $0x928,%rsp ; movq %rsp,%rsi ; callq statfs
          sarl $0x1f,%eax  ; addq $0x928,%rsp ; retq
```

Argument 2 — the caller's `struct statvfs*` — is overwritten with a local stack
address before the call, and there is no copy-out. Confirmed on hardware:

```
/data  RC 0x00000000   every field 0, 128 raw bytes all zero
/user  RC 0xFFFFFFFF   every field 0
```

Same class of SDK/kernel ABI mismatch as `sceKernelStat`'s `st_size`. **A
different mechanism is needed for the capacity check spec D13 requires.**
`statfs` does fill a caller buffer, with FreeBSD's 488-byte struct, and is the
obvious candidate — untested.

---

## 8. The four operations, as sequences

Everything below was executed on hardware. This section is the operational
reference for planning and implementation: it is *how*, where §§1–7 are *what*
and *why*. Nothing here is proposed — it is what ran.

### 8.1 Find the save

```
sceUserServiceInitialize(nullptr)          // 0x80960003 "already initialized" is fine
sceUserServiceGetForegroundUser(&userId)   // fall back to GetInitialUser
sceSaveDataInitialize3(0)

for each of the six known Bloodborne title ids:
    cond = { userId, titleId }             // leave key/order zeroed but never index [0]
    sceSaveDataDirNameSearch(&cond, &result)
    if result.hitNum == 1 -> that is the live save
```

The **AFR title** is a setting and is *not* this value (§3). Accept only an
unambiguous single hit; more than one is a question for the player, not a guess.

### 8.2 Back up

```
sceSaveDataMount(RDONLY)                   // OrbisSaveDataMount, never Mount2
walk the mount with d_type==4 recursion    // sce_sys is a DIRECTORY (§9)
for each file: read through in 64KB chunks to a short read, writing as you go
sceSaveDataUmount()
```

Size every file by **reading it**, never `sceKernelStat` (§9). Write into
`<name>.partial/`, write the manifest **last**, then `sceKernelRename` into
place — a directory without a manifest is not a backup. Reconcile the walked
total against the mount's own reported blocks.

Measured: 26 files, 26,949,914 bytes, byte-exact against a PC copy.

### 8.3 Empty — this is START FRESH

```
sceSaveDataMount(RDWR)                     // no CREATE2, ever
for each ROOT-LEVEL file named userdata* or backup*:
    sceKernelUnlink(mount + "/" + name)
// sce_sys is left alone - a '/' in the relative path means skip it
sceSaveDataUmount()
```

Measured: `REMOVED 11 / REFUSED 0` for `userdata*`, `REMOVED 3 / REFUSED 0` for
`backup*`, leaving four `sce_sys` files and 211,226 bytes. **The game then
started a new playthrough.**

### 8.4 Restore

```
sceSaveDataMount(RDWR)
empty the container first (8.3)            // see below - this is not optional
for each file in the backup EXCEPT sce_sys/*:
    open O_WRONLY|O_CREAT|O_TRUNC, write, close
sceSaveDataUmount()                        // the unmount is what re-seals
re-mount RDONLY and verify against the manifest
```

**Emptying first is mandatory, and not obvious.** `backup*` files accumulate
with play time — three on a fresh save, eleven on a played one. Writing a
three-backup save over an eleven-backup container leaves **eight files of the
previous playthrough** behind, mixed into the restored one. Restore writes the
files it has; it does not remove the ones it doesn't.

Creating files works: after 8.3 removed all eleven `userdata*`, a restore
recreated them and the save loaded. Overwriting works: 13/13 on an intact
container. `sce_sys/*` refuses with EACCES and is skipped by design (§4.1).

Measured: ~15 MB in ~1,009 ms.

### 8.5 Confirm ownership

```
sceUserServiceGetNpAccountId(userId, &account)
read ACCOUNT_ID from <mount>/sce_sys/param.sfo   // SFO format 0x0004, raw bytes
compare
```

They matched exactly on hardware. Ownership is checkable without parsing the
save's game data.

### 8.6 Rules that bind all of the above

* **Bracket every container write.** Read the container before and after; a
  mismatch is conclusive. A match is not proof — the §6 incident left no
  file-level trace.
* **Never set `CREATE2`, never call `sceSaveDataDelete`, never write under
  `sce_sys`.** The first two are not needed and the first is dangerous; the
  third cannot succeed.
* **A failed operation is not a no-op**, and naming a different directory does
  not bound the damage (§6).
* **Check the container is large enough before writing.** `SAVEDATA_BLOCKS` is
  in the unwritable `param.sfo`, so the container's size is fixed.

---

## 9. Traps that cost real time

**`sceKernelStat`'s `st_size` is wrong** — 88 for a genuine 44 KB file
(`app/src/Randomizer/FileIo.cpp`). Size files by reading them through in chunks
until a short read. That also proves the whole file is readable, which a copy
needs anyway.

**A directory opened read-only reads as a file.** It does not fail — it returns
directory data. The first probe reported `sce_sys` as a 32,768-byte "file" and
silently lost 10 MB of a 36 MB save. Use `d_type == 4` and recurse, and always
reconcile a walked total against the mount's own reported size.

**Search ordering is unspecified** unless `cond.key`/`cond.order` are set.
Never index `[0]` and call it "the" save.

**A failed operation is not a no-op.** See §6. "The mount failed, so nothing
happened" was an assumption, and it was wrong.

**Bracket anything risky.** Reading the container immediately before and after an
operation turns "did that hurt?" from a guess into evidence. It is also the one
thing that would have dated the §6 incident.

---

## 10. Save file format, as far as we looked

The files are **not encrypted**. Entropy against a maximum of 8.0:

| File | Entropy | Zeros |
| --- | --- | --- |
| `userdata0000` | 0.27 | 96.8% |
| `userdata0001` | 2.39 | 75.9% |
| `userdata0009` | 5.69 | 34.3% |
| `userdata0010` | 5.67 | 37.3% |

Slots are sparse rather than empty — `userdata0000` still held 41,872 non-zero
bytes across 70 regions. Slot files share a stable header; two saves from
different consoles both begin `41 00 00 00 00 00 00` with matching constants at
offsets 12, 20, 40, 48 and 56.

This was explored only to judge whether a blank save could be *synthesised*. It
cannot, without a reference blank we do not have — and §5 made the question moot.

---

## 11. Still unknown

* **`sceSaveDataDelete`** — never called. Not needed, but also not ruled out if
  a reason appears.
* **Whether `statfs` works** where `statvfs` does not (§7).
* Whether `GetForegroundUser` differs from `GetInitialUser` on a multi-account
  console.
* Whether any Bloodborne install produces **more than one** save directory.
* **`GameInfo::DetectAll` reports three AFR titles on this console** —
  `CUSA03173`, `BossarenaCUSA03173`, `BossarenaCUSA03174`. Save data was found
  only under `CUSA00207`. Which AFR title the Worlds feature should target when
  several exist is a spec question, not a platform one.
