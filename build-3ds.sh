#!/usr/bin/env bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 the starfleet2-3ds authors
# Build the Star Fleet II 3DS app with devkitARM + libctru + 3ds-sdl (SDL 1.2).
#   devkitPro MSYS2 shell (Windows) or Linux:  ./build-3ds.sh
# The .3dsx lives in exactly one place: sd/3ds/starfleet2.3dsx (the SD card layout: the app directly
# in /3ds, the settings, manual and game files in sd/3ds/starfleet2/) -- no other copies are left behind.
set -euo pipefail
cd "$(dirname "$0")"
# Port version for the START menu, taken before anything is generated.
PORT_VERSION="$(git -c safe.directory='*' describe --always --dirty 2>/dev/null || echo unknown)"
: "${DEVKITPRO:=/opt/devkitpro}"; export DEVKITPRO
export DEVKITARM="${DEVKITARM:-$DEVKITPRO/devkitARM}"
export PATH="$DEVKITPRO/tools/bin:$DEVKITARM/bin:$PATH"
echo "== toolchain: $("$DEVKITARM/bin/arm-none-eabi-gcc" --version | head -1)"
[ -f "$DEVKITPRO/libctru/lib/libctru.a" ] || { echo "libctru is missing: install 3ds-dev"; exit 1; }
if [ ! -f "$DEVKITPRO/portlibs/3ds/lib/libSDL.a" ]; then
  echo "== SDL 1.2 for 3DS not found; installing 3ds-sdl"
  (pacman -S --noconfirm --needed 3ds-sdl || dkp-pacman -S --noconfirm --needed 3ds-sdl)
fi
echo "== SDL: $(ls "$DEVKITPRO/portlibs/3ds/lib/libSDL.a")"
if command -v python3 >/dev/null 2>&1; then python3 tools/mkfont.py; else echo "== python3 not found: using the generated sf2ui/fonts.h"; fi
JOBS=$(nproc 2>/dev/null || echo 4)
echo "== building with $JOBS jobs"
make -C dosbox -f Makefile.ctr PORT_VERSION="$PORT_VERSION" -j"$JOBS"
DEST=sd/3ds
mkdir -p "$DEST"
mv -f dosbox/starfleet2.3dsx "$DEST/starfleet2.3dsx"
rm -rf out                                  # old output folder from earlier builds
rm -f sd/3ds/starfleet2/starfleet2.3dsx     # old app location (before Nov 20 2026): never upload it again
ls -la "$DEST/starfleet2.3dsx"
# symbols for reading crash addresses (out-of-tree, never on the SD card): gzipped ELF + demangled nm map
mkdir -p symbols
gzip -9c dosbox/starfleet2.elf > symbols/starfleet2.elf.gz
"$DEVKITARM/bin/arm-none-eabi-nm" -C -n dosbox/starfleet2.elf | gzip -9 > symbols/starfleet2.sym.gz
echo "== version $PORT_VERSION"
echo "== BUILD OK"
