# Bloodborne Randomizer.
#
# Structure mirrors the OpenOrbis toolchain's own samples/hello_world/Makefile
# (same CC/CXX/LD choice, same LDFLAGS shape, same --files "$^" pattern for
# create-gp4), plus a Linux/Darwin/Windows CDIR fallback the shipped samples
# lack. The LIBS line and the toolchain pins below are all hardware-confirmed
# - see docs/ps4-homebrew-findings.md for how each was established.

TITLE       := Bloodborne Randomizer
VERSION     := 00.01
TITLE_ID    := BBRD00001
CONTENT_ID  := IV0000-BBRD00001_00-BBRANDOMIZERMAIN

ifndef OO_PS4_TOOLCHAIN
$(error OO_PS4_TOOLCHAIN is not set. Point it at your OpenOrbis install.)
endif

TOOLCHAIN := $(OO_PS4_TOOLCHAIN)
PROJDIR   := src
INTDIR    := $(PROJDIR)/x64/Debug

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	CDIR := linux
endif
ifeq ($(UNAME_S),Darwin)
	CDIR := macos
endif
ifeq ($(CDIR),)
	CDIR := windows
endif

CC  := clang
CXX := clang++
BIN := $(TOOLCHAIN)/bin/$(CDIR)

# Compile with whatever clang the SDK archive ships, but link with lld 18.1.8
# SPECIFICALLY. Later lld versions produce an .oelf whose SCE-format string
# table the console's runtime loader rejects at launch ("obj_get_str: offset
# out of range"), before main() ever runs - relinking the identical object
# files with 18.1.8 fixes it. Confirmed on real hardware, not by inspection.
# Get the plain archive (NOT the installer - it has no side-by-side support)
# from https://github.com/llvm/llvm-project/releases/tag/llvmorg-18.1.8 and
# extract it anywhere; override LLD18_BIN if it is not at the default below.
LLD18_BIN ?= C:/Users/Fry/LLVM18-portable/bin
LD := $(LLD18_BIN)/ld.lld

# Keep this list minimal: every extra library is another chance to hit the
# bad-NID class of failure, which crashes at load with no output. Add one
# only when the linker actually demands it, found out by trying without it.
#
# -lSceAudioOut is required at link time even though we never call
# SDL_INIT_AUDIO, because libSDL2.a compiles its PS4 audio backend
# (SDL_ps4audio.c) in unconditionally, so its undefined sceAudioOut*
# references have to resolve regardless. -lSceSystemService is what lets
# main.cpp call sceSystemServiceLoadExec("exit") to end the app, since
# returning from main() is not a supported exit path on this SDK.
LIBS := -lc -lkernel -lc++ -lSceVideoOut -lSceSysmodule -lScePad -lSceUserService -lSceAudioOut -lSceSystemService -lSDL2

CFLAGS := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables \
          -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include \
          -isystem $(TOOLCHAIN)/include/SDL2 \
          -O2 -Wall -Wno-unused-function

# gnu++17, not c++17: strict ISO mode hides POSIX declarations behind
# __STRICT_ANSI__, and libc++'s <atomic> unconditionally calls the POSIX
# nanosleep() in its wait/backoff path, so strict mode fails to build.
CXXFLAGS := $(CFLAGS) -isystem $(TOOLCHAIN)/include/c++/v1 -std=gnu++17

LDFLAGS := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr \
           -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

# Recursive (find, not wildcard): M2 introduced src/Platform/ and src/UI/
# subdirectories, and plain $(wildcard $(PROJDIR)/*.cpp) does not descend
# into them.
SRC_CPP := $(shell find $(PROJDIR) -name '*.cpp')
OBJS    := $(patsubst $(PROJDIR)/%.cpp,$(INTDIR)/%.o,$(SRC_CPP))

LIBMODULES := $(wildcard sce_module/*)

_unused := $(shell mkdir -p $(INTDIR))

.PHONY: all clean

all: $(CONTENT_ID).pkg

$(INTDIR)/%.o: $(PROJDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

eboot.bin: $(OBJS)
	$(LD) $(OBJS) -o $(INTDIR)/bbrandomizer.elf $(LDFLAGS)
	$(BIN)/create-fself -in=$(INTDIR)/bbrandomizer.elf -out=$(INTDIR)/bbrandomizer.oelf \
	    --eboot "eboot.bin" --paid 0x3800000000000011

sce_sys/param.sfo: Makefile
	@mkdir -p sce_sys
	$(BIN)/PkgTool.Core sfo_new $@
	$(BIN)/PkgTool.Core sfo_setentry $@ APP_TYPE      --type Integer --maxsize 4  --value 1
	$(BIN)/PkgTool.Core sfo_setentry $@ APP_VER       --type Utf8    --maxsize 8  --value '$(VERSION)'
	$(BIN)/PkgTool.Core sfo_setentry $@ ATTRIBUTE     --type Integer --maxsize 4  --value 0
	$(BIN)/PkgTool.Core sfo_setentry $@ CATEGORY      --type Utf8    --maxsize 4  --value 'gd'
	$(BIN)/PkgTool.Core sfo_setentry $@ CONTENT_ID    --type Utf8    --maxsize 48 --value '$(CONTENT_ID)'
	$(BIN)/PkgTool.Core sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(BIN)/PkgTool.Core sfo_setentry $@ SYSTEM_VER    --type Integer --maxsize 4  --value 0
	$(BIN)/PkgTool.Core sfo_setentry $@ TITLE         --type Utf8    --maxsize 128 --value '$(TITLE)'
	$(BIN)/PkgTool.Core sfo_setentry $@ TITLE_ID      --type Utf8    --maxsize 12 --value '$(TITLE_ID)'
	$(BIN)/PkgTool.Core sfo_setentry $@ VERSION       --type Utf8    --maxsize 8  --value '$(VERSION)'

pkg.gp4: eboot.bin sce_sys/about/right.sprx sce_sys/param.sfo sce_sys/icon0.png $(LIBMODULES)
	@test -f sce_sys/icon0.png || \
	  { echo "ERROR: sce_sys/icon0.png missing."; exit 1; }
	$(BIN)/create-gp4 -out $@ --content-id=$(CONTENT_ID) --files "$^"

$(CONTENT_ID).pkg: pkg.gp4
	$(BIN)/PkgTool.Core pkg_build $< .

clean:
	rm -f $(CONTENT_ID).pkg pkg.gp4 sce_sys/param.sfo eboot.bin \
	    $(INTDIR)/bbrandomizer.elf $(INTDIR)/bbrandomizer.oelf $(OBJS)
