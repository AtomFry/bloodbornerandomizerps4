# Implementation Report — Worlds — milestone 0

**Status: AWAITING HARDWARE TEST**

**Plan:** `docs/features/worlds/plan.md` — milestone 0, "hardware probe"

**Spec:** `docs/features/worlds/spec.md`

**Implemented:** 2026-09-21

---

## 1. What was built

Milestone 0 is diagnostic. It extends the existing temporary scaffolding —
`Platform/SaveDataProbe.{h,cpp}` and `UI/SaveProbeScreen.{h,cpp}` — with a
second entry point, `ProbeSaveDataWrite()`, that runs the milestone's eight
steps in order and prints everything it learns to the screen and to `live.log`.
No production code was added: no `Platform/SaveData`, no `WorldStore`, no
`WorldActivation`, no world UI. `ScreenId`, the Makefile and `LIBS` are
unchanged.

The existing read-only probe (`X`) and backup probe (`SQUARE`) are untouched and
still behave exactly as before. The write probe is a third button, `TRIANGLE`.

| # | Planned change | Status | Where |
| - | -------------- | ------ | ----- |
| 1 | Compile-time scratch save-directory name and a guard aborting every write path, guard state printed | done | `app/src/Platform/SaveDataProbe.cpp:173-185` (constant), `:927-930` (name printed), `:1118-1132` (guard verdict), `:1137-1153` (`scratchAllowed`, re-checked at each of the three save-data write/mount call sites) |
| 2 | Identity: `GetInitialUser`, `GetForegroundUser`, `GetLoginUserIdList`, `GetUserName`, `GetNpAccountId`; `ACCOUNT_ID` from the live save's `param.sfo` via a raw-bytes sibling of `SfoString` | done | `app/src/Platform/SaveDataProbe.cpp:933-983` (identity), `:200-253` (`SfoRaw` / `SfoU64`), `:1158-1216` (live `param.sfo`, printed beside the NP account id with a MATCH/MISMATCH verdict) |
| 3 | `statvfs("/data")` and `statvfs("/user")`, every field raw | done — see §3 | `app/src/Platform/SaveDataProbe.cpp:1210-1256` |
| 4 | Rename a scratch directory with a file under `/data/bbrandomizer/`, confirm the file moved, remove it | done | `app/src/Platform/SaveDataProbe.cpp:1258-1315` |
| 5 | Mount the scratch directory `RDWR\|CREATE2` with `blocks` from the chosen backup's `param.sfo`; copy it in reporting each file's write result | done — backup chosen as in §3 | `app/src/Platform/SaveDataProbe.cpp:1316-1516` |
| 6 | Re-mount `RDONLY`, walk, print count, per-file sizes and total, compare against the source, print a verdict | done | `app/src/Platform/SaveDataProbe.cpp:1517-1620` |
| 7 | `sceSaveDataDelete` the scratch directory, re-search, print whether it is gone | done | `app/src/Platform/SaveDataProbe.cpp:1621-1664` |
| 8 | Time steps 5 and 6 in milliseconds | done | `app/src/Platform/SaveDataProbe.cpp:386` (`NowUs`), `:1509-1514`, `:1613-1618`, `:1665-1671` |

Supporting helpers added to the file's anonymous namespace, all shaped to the
traps `technical-findings.md` §6 records: `TreeWalk` sizes every file by reading
to a short read and recurses on `d_type == 4`; `ReadWholeFile` does the same for
a single file; `MakeParentDirs` creates `sce_sys/` inside the mount, since
`mkdir` does not make parents.

Invariants held:

* The live save directory is mounted exactly once, `RDONLY`, at
  `SaveDataProbe.cpp:1164-1180`. There is no other mount of it in the file.
* Every save-data write path names `kScratchDirName`, a compile-time constant,
  and calls `scratchAllowed()` immediately before firing. `SPRJ0005` is never
  typed anywhere.
* No privileged path. The write probe touches `/data/bbrandomizer`, save mounts,
  and nothing else. No `/system_data/`, no libjbc, no sandbox escape.
* `LIBS` gained nothing; `app/Makefile` is unchanged.
* Layering: all orbis calls stay in `Platform/`; `SaveProbeScreen` calls only
  `ProbeSaveDataWrite()`.

---

## 2. Deviations from the plan

All six are in the plan's §10. Summarised:

1. **Step order.** The steps run 1a (print the scratch name) → 2 (identity,
   then the title sweep) → 1b (guard verdict) → 2c (live `ACCOUNT_ID`) → 3 → 4 →
   5 → 6 → 7 → 8. The guard compares the scratch name against the directories
   the search returned, so its verdict cannot be printed before that search. Its
   done-condition — "the guard's state prints in the probe output" — is met;
   only the position of the line moved. Every block is labelled with its plan
   step number so the output still reads in the plan's terms.

2. **A second entry point on a third button.** The plan's §7 lists the probe's
   steps but not how the developer starts them. `ProbeSaveDataWrite()` is bound
   to `TRIANGLE`; `X` and `SQUARE` keep their existing meanings, which matters
   because `SQUARE` is what produces the backup step 5 restores from.

3. **Step 3 prints named fields plus a hex dump, into an oversized buffer.** See
   §3 below.

4. **Step 5's backup selection rule.** See §3 below.

5. **`SaveDataProbe.h`'s delete list updated** to say `-lSceSaveData` stays, per
   §7 milestone 6 step 6. Comment only.

6. **The `PARAMS` `CUSA#####` scan was not implemented.** It is the §3.3 hazard
   row for save-title discovery, and discovery is milestone 1 step 2. The piece
   milestone 1 needs from milestone 0 — a param.sfo reader that handles format
   `0x0004` — is in place as `SfoRaw`.

---

## 3. Decisions the plan left open

### 3.1 Which user id the probe scopes its searches to

Step 2 says to call all five identity functions and print them; it does not say
which id the search and mount below should use. §4.4 refuses when there is no
foreground user, but that is milestone 1's production rule and refusing here
would make the probe unrunnable on a console where the call happens to fail —
which is one of the things the probe exists to find out.

Taken: use the foreground user when `sceUserServiceGetForegroundUser` succeeds
and returns a valid id; otherwise fall back to the initial user and print
`(INITIAL - FOREGROUND UNAVAILABLE)`; abort only when neither is usable. The
chosen id is printed on its own line.

### 3.2 Step 3's "every field printed raw"

Taken: all eleven named `struct statvfs` fields, then a 128-byte dump of the
buffer as sixteen hex words, into a **1 KB zeroed arena** cast to
`struct statvfs*` rather than a bare local struct.

Why: the named fields are musl's Linux layout, which is exactly what is in
doubt. The hex words are what actually landed in memory and are the thing a
judgement can be made from. The oversized arena exists because if the kernel
writes a struct larger than musl's 112 bytes — FreeBSD's `struct statfs` is 488
— a bare local would be a stack smash rather than a result; with the arena it
shows up in the dump.

Nothing is interpreted. The probe prints and moves on.

### 3.3 Which local backup step 5 restores from

The plan says "the chosen local backup's `param.sfo`" without saying how it is
chosen.

Taken: among the subdirectories of `/data/bbrandomizer/SaveBackups` whose name
begins with `<live save title id>_`, use the lexicographically greatest. Backup
names are `<title>_<dir>_%Y%m%d-%H%M%S`, so greatest is newest, and the title
prefix keeps the restore in the same save container. The candidate count and the
chosen name are both printed. With no candidate, steps 5–7 are skipped with a
line telling the developer to press `SQUARE` first.

### 3.4 A leftover scratch directory trips the guard

The guard is specified as aborting "if it equals a discovered live directory
name", and §E5.1 as "if that constant ever equals a directory the search
returned". A scratch directory left behind by a failed delete *is* a directory
the search returns, so on the next run the guard trips and every write step
refuses.

Taken: implement it literally and leave it tripped. The guard cannot tell a
leftover from a collision and guessing is not what a guard is for. The output
names the match and points at Settings → Application Saved Data Management,
which the plan's own gate names as the recovery.

### 3.5 Selecting the live save without indexing

§3.1 forbids selecting a search result by index. The probe accepts a live
directory only when the first title with any hits has **exactly one**; anything
else prints every name found, sets a flag, and skips steps 5–7 (that is the
plan's "more than one save directory" stop condition, surfaced as probe output
rather than a crash). Step 7's re-search scans the returned names for
`kScratchDirName` and for the live name by string comparison. `cond.key` and
`cond.order` are left at their defaults, and nothing reads meaning from a
position.

---

## 4. Verification run

| Check | Command | Result |
| ----- | ------- | ------ |
| Clean rebuild | `cd app && make clean && make` | `IV0000-BBRD00001_00-BBRANDOMIZERMAIN.pkg` built, 7,143,424 bytes; `eboot.bin` 3,708,752 bytes |
| Compiler warnings | changed objects deleted and rebuilt | none (`-Wall` is on) |
| Settings UI mirror | `python app/tools/settings_ui_verify.py` | 40/40 passing |
| Scroll geometry | `python app/tools/ui_scroll_verify.py` | all geometry and scroll properties PASSED |
| Pool / defaults mirror | `python app/tools/pool_verify.py selftest data/vanilla/dvdroot_ps4` | 88/88 passing |
| New screen strings | `settings_ui_verify.load_atlas()` / `width()` at scale 3 | widest new body line 946 px against the 120→1680 band; footer 838 px against 1920; every glyph present in the atlas |

**Not run**, and why:

* `worlds_verify.py` — does not exist yet; it is milestone 1 step 7.
* The output-parity suite (`boss_verify`, `treasure_verify`, `drops_verify`,
  `starting_weapons_verify`, `caged_dogs_verify`, `easy_modes_verify`,
  `hunter_tools_verify`, `mergo_darkness_verify`, `itemdata_verify`) — milestone
  0 changes no randomizer code, no settings and no tables. `pool_verify.py`'s
  selftest was run anyway as the cheapest check that the defaults chain is
  untouched.
* H1, H2, H3 — hardware. See §6.

---

## 5. What this does not prove

Everything this milestone exists to answer. A clean cross-compile shows the
calls type-check and the binary links; it says nothing about whether
`RDWR|CREATE2` mounts, whether `sceSaveDataDelete` returns 0, whether
`sceUserServiceGetForegroundUser` returns what `technical-findings.md` §1.2
expects, or whether `statvfs` produces a usable number. Those are exactly the
unknowns in findings §7 and only the PS4 can close them.

One thing **was** established statically, and the developer should know it
before running the probe:

> `statvfs` in this toolchain's `libc.a` cannot fill the caller's struct.
> Disassembling `statvfs.lo` shows the body allocating a 0x928-byte local,
> loading `%rsi` with `%rsp` — discarding the caller's buffer pointer — calling
> `statfs`, and returning `eax >> 31`. There is no copy-out step. So step 3's
> named fields and hex dump will almost certainly read as zeros, `statvfs`'s
> return code will be the only signal, and §4.4's "if milestone 0 shows its
> fields are sane" branch looks already decided against.

That is a prediction from static reading, not a result, and the probe still runs
the call — a prediction that the hardware contradicts would itself be worth
knowing. It is **not** treated as the plan's `statvfs`-unusable stop condition,
because that condition requires the fallback capacity probe to have failed too,
and the fallback is milestone 1's work. `libc.a` also exports `statfs`, which
*does* fill a caller-supplied buffer with the kernel's FreeBSD `struct statfs`;
whether §4.4 should use it instead of the write-a-file fallback is a plan
question, not an implementation one, and was deliberately left alone.

---

## 6. Hardware test handoff

Install the `.pkg` from `app/`. The probe is main menu → **SAVE DATA PROBE
(TEST)**.

**Before anything: confirm you still have the verified byte-exact backup on the
PC** (`data/Save Backups/CUSA00207_SPRJ0005_20260921-032915`, 26 files,
26,949,914 bytes). Nothing below should need it. It is the reason this is
runnable at all.

### Order

1. **Press `SQUARE` first** — the existing read-only probe plus backup. This
   writes `/data/bbrandomizer/SaveBackups/CUSA00207_SPRJ0005_<stamp>/` and is
   what the write probe restores from. Confirm it ends
   `BACKUP COMPLETE - 0 ERROR(S)`. If you already have a backup on the console
   from the earlier session you can skip this, but a fresh one costs a minute.
2. Leave the screen (`O`) and re-enter it — each visit runs one probe.
3. **Press `TRIANGLE`.** The screen will freeze for as long as steps 5 and 6
   take; it copies ~27 MB in and reads ~27 MB back inside one frame. `live.log`
   is written line by line throughout, so a hang or a crash still leaves the
   trail.
4. Scroll with Up/Down and read the whole output, or pull `live.log`.

### What to look for, step by step

| Step | Pass looks like |
| ---- | --------------- |
| 1 | `SCRATCH SAVE DIR BBRRESTORETEST`, and later `ARMED - BBRRESTORETEST MATCHES NONE OF n DISCOVERED DIR(S)` |
| 2 | `GETFOREGROUNDUSER 0x00000000` with a valid id; `GETNPACCOUNTID` non-zero; `PARAM.SFO ACCOUNT_ID` and `GETNPACCOUNTID` printing the same 64-bit value, then `MATCH - NP ACCOUNT ID IS USABLE FOR OWNERSHIP` (this settles **P14**) |
| 3 | Numbers print, sane or not. Per §5 expect zeros; record them verbatim either way |
| 4 | `SCEKERNELRENAME 0x00000000`, `FILE MOVED WITH THE DIRECTORY - CONTENT MATCHES`, `OLD PATH GONE`, then a clean cleanup line |
| 5 | `MOUNTED AT /savedataN`, then one `... BYTES WRITTEN OK` line per file, `WROTE 26 OF 26 FILE(S)`, `DEST OPEN REFUSED 0, FAILED 0`, `UNMOUNT 0x00000000` (this settles **P15**) |
| 6 | `RE-MOUNTED AT /savedataN`, 26 files walked, `26949914 BYTES`, and `VERDICT MATCH - EVERY FILE AND EVERY BYTE ACCOUNTED FOR`. `MOUNT REPORTS 1136 BLOCKS (36352 KB)` reconciles the allocation |
| 7 | `SCESAVEDATADELETE ... 0x00000000`, `SCRATCH DIRECTORY GONE`, `LIVE DIRECTORY SPRJ0005 STILL PRESENT` |
| 8 | Two millisecond figures |

### What failure looks like — the two that halt the feature

**Step 5 fails.** Any of:

* `MOUNT FAILED 0x........ - REQUIRED BLOCKS n` — `RDWR|CREATE2` is refused for
  a cross-title container, or the container size read from the backup is too
  small. The `REQUIRED BLOCKS` figure is the useful part.
* `DEST OPEN REFUSED` on some or all files — the mount is writable but the
  service rejects the write. The *pattern* is the finding: if `sce_sys/*` is
  refused and `userdata*` accepted, P15's fallback ("restore only the files the
  game wrote plus `sce_sys/param.sfo`") is the answer; if everything is refused,
  restore does not work from homebrew.
* Files report `n OF m BYTES THEN ...` — a partial write.

**Step 7 fails.** `SCESAVEDATADELETE ... 0x........` non-zero, or
`SCRATCH DIRECTORY STILL PRESENT` after it.

In either case **stop — do not start milestone 1.** The plan's gate makes both
of these hard stops: the design depends on restore and delete working. Report
the error code.

### If the scratch directory survives

Remove `BBRRESTORETEST` under Bloodborne in **Settings → Application Saved Data
Management → Saved Data in System Storage → Delete**. It is not the game's save;
Bloodborne uses `SPRJ0005`, which the probe never names. Leaving it in place
also means the guard will trip on the next run and the write steps will refuse —
that is intended, not a bug.

### If the guard trips on a first run

`TRIPPED - BBRRESTORETEST MATCHES A DISCOVERED DIRECTORY` on a console that has
never run the write probe would mean the constant collides with a real
directory. That is a plan stop condition. Report it and do not change the
constant to route around it.

### After the run

Record the output in `technical-findings.md` §7 per the plan's gate. It settles
**P14** (account scoping, from step 2) and **P15** (which files a restore
writes, from step 5's per-file results).

---

## 7. Stop point

Milestone 0's completion gate: the eight steps are implemented, the clean
rebuild produced the `.pkg`, and the mirrors that could regress are green. No
stop condition fired during implementation.

Milestone 0 is handed over for **H1, H2 and H3**. Milestone 1 — `Platform/SaveData`
— must not begin until those have run, and must not begin at all if step 5 or
step 7 fails.
