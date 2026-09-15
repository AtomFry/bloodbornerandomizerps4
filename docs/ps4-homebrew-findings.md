# PS4 homebrew — what we know

Hardware-confirmed findings about building native homebrew for a 9.00 GoldHEN
PS4, accumulated while bringing this project up. Everything here was established
by running code on the real console, not by reading documentation — several of
these contradict what the documentation and the SDK headers imply.

Keep this current. When a belief here is disproved, correct the entry rather
than adding a contradicting one below it.

---

## 1. The filesystem and the sandbox

**`/app0` is per-process.** Inside our homebrew, `/app0` is *our own* package
mount. Bloodborne's `/app0` exists only inside Bloodborne's sandbox, only while
Bloodborne is running. Successfully reading `/app0` from our process proves
nothing about reaching the game — it means we read ourselves.

**Installed game content is unreachable.** Bloodborne's files live in an
encrypted PFS image that is mounted only inside the title's own sandbox. There
is no documented API for a separate process to mount it on demand. Probing
nine candidate content roots plus a depth-limited hunt for `dvdroot_ps4` found
nothing, with the game both stopped and suspended.

**Consequence for this project:** vanilla game data has to be put on the console
by hand, once, over FTP. That is why the app expects a vanilla `dvdroot_ps4` at
`/data/bbrandomizer/VanillaSource/dvdroot_ps4` and reads from there.

**`/data/GoldHEN/AFR/<TITLEID>/` is fully writable from an unescalated homebrew
process.** Full CRUD — create, read, write, rename, delete — confirmed against a
scratch file on real hardware. **No sandbox escape is required.** This is the
single most important finding in the project: it is why no libjbc dependency,
no credential manipulation, and no jailbreak-adjacent code exists anywhere in
this codebase, and why none should be added.

**FreeBSD `open(2)` flag values, not Linux ones.** The PS4 kernel is
FreeBSD-derived and `sceKernelOpen` expects BSD values. musl's `<fcntl.h>`
carries Linux values (`O_CREAT` = 0100 octal = 64, versus BSD's 0x0200). Pass
explicit BSD values to every `sceKernel*` call:

| Flag | Value |
|---|---|
| `O_RDONLY` | `0x0000` |
| `O_WRONLY` | `0x0001` |
| `O_RDWR` | `0x0002` |
| `O_APPEND` | `0x0008` |
| `O_CREAT` | `0x0200` |
| `O_TRUNC` | `0x0400` |
| `O_EXCL` | `0x0800` |
| `O_DIRECTORY` | `0x00020000` |

`st_mode` bits are the same on BSD and Linux (`IFMT` 0xF000, `IFDIR` 0x4000,
`IFREG` 0x8000, `IFLNK` 0xA000).

**FreeBSD's on-disk `dirent`, as returned by `getdents(2)`.** Declare this
locally rather than relying on an SDK typedef — the SDK's field names vary
between toolchain versions, while this layout is stable across every firmware:

```c
struct BsdDirent {
    uint32_t d_fileno;
    uint16_t d_reclen;
    uint8_t  d_type;
    uint8_t  d_namlen;
    char     d_name[256];
};
```

`d_type` values: UNKNOWN 0, FIFO 1, CHR 2, DIR 4, BLK 6, REG 8, LNK 10, SOCK 12.

**`OrbisKernelStat`'s `st_size` is unreliable.** It returned 88 for a genuine
44KB file on real hardware, while `st_mode` checks have always been correct.
Read whole files in growing chunks until a short read rather than sizing off
`st_size`. The header's layout does not necessarily match the real kernel ABI
field-for-field — treat any `OrbisKernelStat` field this project has not
already hardware-validated with suspicion.

---

## 2. Toolchain

Built with the OpenOrbis PS4 toolchain; `OO_PS4_TOOLCHAIN` must point at the
directory that directly contains `bin/`, `include/`, `lib/`, `link.x` and
`samples/`. WSL2 Ubuntu is the best-trodden path — OpenOrbis ships native Linux
binaries for its packaging tools.

Before debugging anything in this project, confirm the toolchain itself works:

```bash
cd "$OO_PS4_TOOLCHAIN/samples/hello_world"
make
```

If that does not produce a `.pkg`, nothing here will work either. Its Makefile
is the reference every Makefile in this project is modelled on.

**Compile with clang 22, link with lld 18.1.8 — specifically.** The SDK archive
is named "toolchain-llvm-18" but the current LLVM.org release is 22.x.
Compiling with 22 is fine; every source file compiles clean under it. *Linking*
with 22's `ld.lld` is not: it produces an `.oelf` whose SCE-format string table
the console's runtime loader rejects at launch (`obj_get_str: offset out of
range`), before `main()` ever runs. Relinking the identical object files with
lld 18.1.8 fixed it, confirmed on real hardware rather than by inspection.

Get the plain archive from the
[llvmorg-18.1.8 release](https://github.com/llvm/llvm-project/releases/tag/llvmorg-18.1.8)
(`clang+llvm-18.1.8-x86_64-pc-windows-msvc.tar.xz`) and extract it anywhere —
**not the installer**, which does not support side-by-side versions.

**`-std=gnu++17`, not `-std=c++17`.** Strict ISO mode hides POSIX declarations
behind `__STRICT_ANSI__`, and libc++'s `<atomic>` unconditionally calls POSIX
`nanosleep()` in its wait/backoff path. Strict mode fails to build.

**Always do a full clean rebuild after any class or struct layout change.** A
stale partial rebuild once produced a real heap-corruption SIGSEGV on hardware.

---

## 3. Packaging

**`create-fself` needs `--eboot "eboot.bin"`.** Without it, only the
intermediate `.oelf` is produced and no `eboot.bin` lands in the project root.

**`create-gp4` takes `--files "$^"`** — a plain space-separated list of paths,
used as both source and in-package destination. There is no `"src dst;src dst"`
syntax; that was invented once and does not work.

Every PKG needs an icon at `sce_sys/icon0.png` or the build fails.

Install by copying the `.pkg` to `/data/pkg/` over FTP (GoldHEN's FTP server is
on port 2121) and installing via Settings → Debug Settings → Package Installer.

---

## 4. SDL2 on this console

Proven working in isolation: `SDL_Init(VIDEO | JOYSTICK)` → window → software
renderer → draw → present → read a real controller button → exit cleanly.

**Present with `SDL_UpdateWindowSurface()`, not `SDL_RenderPresent()`.** The
renderer is created via `SDL_CreateSoftwareRenderer(windowSurface)`, so it draws
into the window's own CPU-side pixel buffer rather than a GPU swap chain —
pushing those pixels to the screen is a window-surface operation.

**`-lSceAudioOut` is required at link time even with no audio.** `libSDL2.a`
compiles its PS4 audio backend (`SDL_ps4audio.c`) in unconditionally, so its
undefined `sceAudioOut*` references must resolve whether or not that path ever
runs.

**`-lSceSystemService` is required to exit.** Returning from `main()` is not a
supported exit path on this SDK — every official sample calls
`sceSystemServiceLoadExec("exit")` instead.

**Pad input via the low-level `SDL_Joystick` API**, reading
`SDL_JOYBUTTONDOWN` / `ev.jbutton.button`. `PAD_BUTTON_CROSS = 0`.
`SDL_GameController` exists in this SDK but needs a mapping-database entry for
this pad that the proven samples do not use.

**FreeType has never been proven alongside SDL2.** It was made to work only on
the separate `Scene2D` raw-framebuffer rendering path. That is why this project
draws text with a hand-authored 8x8 bitmap font instead of taking the FreeType
dependency.

**Log crash-resiliently: open, write one line, `fsync`, close, every time.** No
buffering — there is nothing to lose if the process dies mid-operation, and a
process that dies mid-operation is the normal case while bringing something up.

---

## 5. Reading errors

Errors are worth reporting twice, because which half carries the truth varies
by call:

```
stat failed: ret=0x80020002 sce=2 ENOENT | errno=2 ENOENT
```

- `ret=0x8002xxxx` — SCE's encoding; the **low 16 bits are a BSD errno**. A raw
  result of `0x80020002` is ENOENT. A plain `-1` means "check errno".
- `errno=` — what musl saw.

**Decode with a BSD errno table, not `strerror()`.** musl's table is Linux's,
and several codes differ — `ENAMETOOLONG` is 63 on BSD and 36 on Linux.

Where `sceKernel*` and POSIX backends disagree for the same path, that
disagreement is itself information about whether OpenOrbis's musl translates BSD
open flags correctly.

---

## 6. Bloodborne title IDs

| Title ID | Region / edition |
|---|---|
| `CUSA00900` | USA |
| `CUSA00207` | Australia |
| `CUSA00208` | United Kingdom |
| `CUSA01363` | Asia |
| `CUSA03014` | Japan / The Old Hunters Edition |
| `CUSA03173` | Europe / GOTY |

Detect an installed title by walking `/user/app`, not with `AppInstUtil` — see
below.

---

## 7. Dead ends, recorded so they are not retried

**`sceAppInstUtilInitialize()` crashes the binary at load.** On this SDK plus
firmware 9.00, calling it causes the runtime loader to reject the whole binary
with `PRX_NOT_RESOLVED_FUNCTION` before `main()` runs — the header's declared
NID does not match what `libSceAppInstUtil.sprx` actually exports on this
firmware. Verified by decoding the crash's reported NID
(`0xE78D25A2D3BBA071`) into PS4's symbol-name base64 form and matching it
against the binary's own dynamic symbol table. **No linker choice fixes this.**
Filesystem-based detection is unaffected and was always the more reliable of the
two anyway.

**libjbc sandbox escape — investigated, confirmed unnecessary, deliberately not
used.** AFR access works without it (§1). Since the only thing the project needs
is ordinary homebrew filesystem access, taking a privilege-escalation dependency
would add risk and scope for no capability. Do not add it back.

**Link fewer libraries, not more.** Each one is a chance to repeat the
`AppInstUtil` bad-NID class of failure, which manifests as a launch-time crash
with no output. Add a library only when the linker actually demands it, and find
that out by trying without it first.
