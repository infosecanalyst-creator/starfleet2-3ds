#!/usr/bin/env bash
# Build the Star Fleet II 3DS app with devkitARM + libctru + 3ds-sdl (SDL 1.2).
#   devkitPro MSYS2 shell (Windows) or Linux:  ./build-3ds.sh
# The .3dsx lives in exactly one place: sd/3ds/starfleet2/ (the SD card layout, with the
# settings, manual and game files) -- no other copies are left behind.
set -euo pipefail
cd "$(dirname "$0")"
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
make -C dosbox -f Makefile.ctr -j"$JOBS"
DEST=sd/3ds/starfleet2
mkdir -p "$DEST"
mv -f dosbox/starfleet2.3dsx "$DEST/starfleet2.3dsx"
rm -rf out                                  # old output folder from earlier builds
ls -la "$DEST/starfleet2.3dsx"
echo "== BUILD OK"
